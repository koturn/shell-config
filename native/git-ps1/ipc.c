/*!
 * @brief IPC utilities.
 * @author koturn
 * @file ipc.c
 */
#include "ipc.h"
#include "logging.h"
#ifdef USE_POSIX_IPC
#    include "sha256.h"
#endif  // defined(USE_POSIX_IPC)

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_POSIX_IPC
#    include <semaphore.h>
#    include <sys/mman.h>
#    include <fcntl.h>
#    include <unistd.h>
#else
#    include <sys/ipc.h>
#    include <sys/sem.h>
#    include <sys/shm.h>
#endif // defined(USE_POSIX_IPC)


#ifndef USE_POSIX_IPC
//! Shared memory ID.
static const int kFtokId = 'G';

/*!
 * @brief Union for `semctl()`.
 */
typedef union {
    //! Value for `SETVAL`.
    int val;
    //! Buffer for `IPC_STAT` or `IPC_SET`.
    struct semid_ds *buf;
    //! Array for `GETALL` or `SETALL`.
    unsigned short  *array;
} SemData;
#endif  // !defined(USE_POSIX_IPC)


/*!
 * @brief Create IPC key.
 * @param [in] filePath  File path to create IPC key.
 * @param [out] pIpcKey  IPC key destination.
 * @return Zero if success, otherwise non-zero.
 */
int createIpcKey(const char *filePath, IpcKey *pIpcKey)
{
#ifdef USE_POSIX_IPC
    char hash[kSha256StrLength + 1];
    sha256str(hash, (const unsigned char *)filePath, strlen(filePath));

    int iChars = snprintf(pIpcKey->name, sizeof(pIpcKey->name), "git-ps1-%s", hash);
    if (iChars >= (int)sizeof(pIpcKey->name)) {
        return 1;
    }
#else
    key_t key = ftok(filePath, kFtokId);
    if (key == -1) {
        printLastError("ftok() failed");
        return errno;
    }

    pIpcKey->value = key;
#endif  // defined(USE_POSIX_IPC)

    return 0;
}


/*!
 * @brief Create/Initialize or Get exist semaphore key.
 * @param [in] pIpcKey  IPC key.
 * @param [out] pSemKey  Semaphore key destination.
 * @return Zero if success, otherwise non-zero.
 */
int getOrCreateSemaphre(const IpcKey *pIpcKey, SemKey *pSemKey)
{
#ifdef USE_POSIX_IPC
    sem_t *sem = sem_open(pIpcKey->name, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        printLastError("shm_open() failed");
        return errno;
    }

    pSemKey->value = sem;
#else
    int semId = semget(pIpcKey->value, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if (semId == -1) {
        semId = semget(pIpcKey->value, 1, 0666 | IPC_CREAT);
        if (semId == -1) {
            printLastError("semget() failed");
            return -1;
        }
    } else {
        SemData ctlarg = { .val = 1 };
        if (semctl(semId, 0, SETVAL, ctlarg) == -1) {
            printLastError("semctl() failed");
            return -1;
        }
    }

    pSemKey->value = semId;
#endif  // defined(USE_POSIX_IPC)

    return 0;
}


/*!
 * @brief Delete semaphore.
 * @param [in] pIpcKey  IPC key.
 * @return Zero if success, otherwise non-zero.
 */
int deleteSemaphore(const IpcKey *pIpcKey)
{
#ifdef USE_POSIX_IPC
    return sem_unlink(pIpcKey->name);
#else
    int semId = semget(pIpcKey->value, 1, 0);
    if (semId == -1) {
        printLastError("shmget() failed");
        return errno;
    }

    if (semctl(semId, 1, IPC_RMID, NULL) != 0) {
        printLastError("semctl() failed");
        return errno;
    }

    return 0;
#endif  // defined(USE_POSIX_IPC)
}


/*!
 * @brief Try to wait semaphore.
 * @param [in] pSemKey Semaphore key.
 * @return true if the semaphore was successfully acquired, otherwise false.
 */
bool tryWaitSemaphre(const SemKey *pSemKey)
{
#ifdef USE_POSIX_IPC
    return sem_trywait(pSemKey->value);
#else
    struct sembuf op = {
        .sem_num = 0,
        .sem_op = -1,
        .sem_flg = IPC_NOWAIT
    };
    return semop(pSemKey->value, &op, 1) == 0;
#endif  // defined(USE_POSIX_IPC)
}


/*!
 * @brief Post to semaphore.
 * @param [in] pSemKey  Semaphore key.
 * @return Zero if success, otherwise non-zero.
 */
int postSemaphore(const SemKey *pSemKey)
{
#ifdef USE_POSIX_IPC
    return sem_post(pSemKey->value);
#else
    struct sembuf op = {
        .sem_num = 0,
        .sem_op = 1,
        .sem_flg = 0
    };
    if (semop(pSemKey->value, &op, 1) == -1) {
        printLastError("semop() failed");
        return errno;
    }

    return 0;
#endif  // defined(USE_POSIX_IPC)
}


/*!
 * @brief Create or Get exist shared memory ID.
 * @param [in] pIpcKey  IPC key.
 * @param [in] shmSize  Size of shared memory.
 * @return Shared memory ID (-1 means error).
 */
int getOrCreateSharedMemory(const IpcKey *pIpcKey, size_t shmSize)
{
#ifdef USE_POSIX_IPC
    int shmId = shm_open(pIpcKey->name, O_RDWR | O_CREAT, 0666);
    if (shmId == -1) {
        printLastError("shm_open() failed");
        return -1;
    }

    int ret = ftruncate(shmId, (off_t)shmSize);
    if (ret == -1) {
        printLastError("shm_open() failed");
        return -1;
    }

    return shmId;
#else
    int shmId = shmget(pIpcKey->value, shmSize, 0666 | IPC_CREAT);
    if (shmId == -1) {
        printLastError("shmget() failed");
        return -1;
    }

    return shmId;
#endif  // defined(USE_POSIX_IPC)
}


/*!
 * @brief Delete shared memory.
 * @param [in] pIpcKey  IPC key.
 * @return Zero if success, otherwise non-zero.
 */
int deleteSharedMemory(const IpcKey *pIpcKey)
{
#ifdef USE_POSIX_IPC
    return shm_unlink(pIpcKey->name);
#else
    int shmId = shmget(pIpcKey->value, 0, 0);
    if (shmId == -1) {
        printLastError("shmget() failed");
        return errno;
    }

    if (shmctl(shmId, IPC_RMID, NULL) != 0) {
        printLastError("shmctl() failed");
        return errno;
    }

    return 0;
#endif  // defined(USE_POSIX_IPC)
}


/*!
 * @brief Attach shared memory.
 * @param [in] shmId  Shared memory ID.
 * @param [in] shmSize  Size of shared memory.
 * @return Shared memory ID (-1 means error).
 */
void *attachSharedMemory(int shmId, size_t shmSize)
{
#ifdef USE_POSIX_IPC
    return mmap(NULL, shmSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmId, 0);
#else
    (void)shmSize;
    return shmat(shmId, 0, 0);
#endif  // defined(USE_POSIX_IPC)
}


/*!
 * @brief Dettach shared memory.
 * @param [in] pShm  Pointer to attached shared memory.
 * @param [in] shmSize  Size of shared memory.
 * @return Shared memory ID (-1 means error).
 */
int dettachSharedMemory(void *pShm, size_t shmSize)
{
#ifdef USE_POSIX_IPC
    return munmap(pShm, shmSize);
#else
    (void)shmSize;
    return shmdt(pShm);
#endif  // defined(USE_POSIX_IPC)
}
