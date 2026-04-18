/*!
 * @brief IPC utilities.
 * @author koturn
 * @file ipc.h
 */
#ifndef IPC_H
#define IPC_H

#include <sys/types.h>
#include <stdbool.h>
#include <stddef.h>
#ifdef USE_POSIX_IPC
#    include <semaphore.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


/*!
 * @brief IPC key struct.
 */
typedef struct {
#ifdef USE_POSIX_IPC
    //! Name for semaphore and shared memory.
    char name[256];
#else
    //! The value returned by `ftok()`.
    key_t value;
#endif  // defined(USE_POSIX_IPC)
} IpcKey;


/*!
 * @brief Semaphore key struct.
 */
typedef struct {
#ifdef USE_POSIX_IPC
    //! Pointer to semaphore.
    sem_t* value;
#else
    //! Semaphore ID.
    int value;
#endif  // defined(USE_POSIX_IPC)
} SemKey;


int createIpcKey(const char *source, IpcKey *pIpcKey);

int getOrCreateSemaphre(const IpcKey *pIpcKey, SemKey *pSemKey);
int deleteSemaphore(const IpcKey *pIpcKey);
bool tryWaitSemaphre(const SemKey *pSemKey);
int postSemaphore(const SemKey *pSemKey);

int getOrCreateSharedMemory(const IpcKey *pIpcKey, size_t shmSize);
int deleteSharedMemory(const IpcKey *pIpcKey);
void *attachSharedMemory(int shmId, size_t shmSize);
int dettachSharedMemory(void *pShm, size_t shmSize);


#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */


#endif  /* IPC_H */
