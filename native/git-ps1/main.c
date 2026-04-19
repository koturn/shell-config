/*!
 * @brief A utility that caches Git status in shared memory and retrieves it.
 *
 * @author koturn
 * @file main.c
 */
#ifndef _DEFAULT_SOURCE
#    define _DEFAULT_SOURCE
#endif  // !defined(_DEFAULT_SOURCE)

#include "ipc.h"
#include "gitop.h"
#include "logging.h"
#ifdef USE_POSIX_IPC
#    include "sha256.h"
#endif  // USE_POSIX_IPC

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include <getopt.h>
#include <unistd.h>

#ifdef USE_MULTI_THREAD
#    include <pthread.h>
#endif  // defined(USE_MULTI_THREAD)

#define AUTHOR "koturn"
#define VERSION "0.1.0"
#define MIN(x, y)  ((x) < (y) ? (x) : (y))


/*!
 * @brief Name type constants.
 */
typedef enum {
    //! Means detection failed.
    kNone = 0,
    //! Means branch name.
    kBranch = 1,
    //! Means tag name.
    kTag = 2,
    //! Means commit hash.
    kCommitHash = 3
} NameType;

/*!
 * @brief Prompt type constants.
 */
typedef enum {
    //! Means nothing.
    kNoColorPrompt = -1,
    //! Means other.
    kDefaultPrompt = 0,
    //! Means zsh.
    kZshPrompt = 1,
    //! Means tmux.
    kTmuxPrompt = 2
} PromptType;


/*!
 * @brief Optional parameters.
 */
typedef struct {
    //! Wait time in milliseconds.
    int waitTime;
    //! Working directory.
    const char *workDir;
    //! Do reflesh or not.
    bool doReflesh;
    //! Do read and exit or not.
    bool doReadOnly;
    //! Do not use shared memory cache.
    bool noCache;
    //! Prompt type.
    PromptType promptType;
} CmdParam;


static void parseArguments(CmdParam *pParam, int argc, char *argv[]);
static void showUsage(FILE *stream, const char *progName);
static void showVersion(FILE *stream);
static int tryParseInt(const char *numstr, int *pOutVal);
#ifdef __GNUC__
__attribute__((pure))
#endif  // defined(__GNUC__)
static const char *skipSpace(const char *str);
static int onLockAquired(int waitTime, PromptType promptType);
static int onLockNotAquired(PromptType promptType);
static int readSharedMemory(int shmId, char *data, size_t bufSize);
static int writeSharedMemory(int shmId, const unsigned char *data, size_t dataLen);
static int printPs1(FILE *stream, PromptType promptType, int shmId, const char *statstr);
static void printDefaultPs1(FILE *stream, NameType nameType, const char *name, const char *statstr);
static void printZshPs1(FILE *stream, NameType nameType, const char *name, const char *statstr);
static void printTmuxPs1(FILE *stream, NameType nameType, const char *name, const char *statstr);
static void sigHandler(int signum);
static void releaseResource(bool shouldSemPost);
static NameType getDisplayName(char *buf, size_t bufSize);
static int getGitStatString(char *buf, size_t bufSize);
#ifdef USE_MULTI_THREAD
static void *thProcGitCheckChanged(void *args);
static void *thProcGitCheckStaged(void *args);
static void *thProcGitCheckUntracked(void *args);
#endif  // defined(USE_MULTI_THREAD)


//! Name buffer size.
static const size_t kNameBufSize = 1024;
//! Default wait time in milliseconds.
static const int kDefaultWaitTime = 200;
//! Polling cycle in milliseconds.
static const int kPollCycle = 1;
//! Shared memory size.
static const int kShmSize = 32;
//! Target process ID.
static pid_t g_targetPid;
//! Semaphore ID.
static SemKey g_semKey;
//! Shared memory ID.
static int g_shmId;
//! A flag should decrement semaphore counter or not.
static bool g_shouldSemPost = false;


/*!
 * @brief Entry point of the program
 * @param [in] argc  A number of command-line arguments
 * @param [in] argv  Command line arguments
 * @return  Exit-status
 */
int main(int argc, char *argv[])
{
    int ret;

    CmdParam param = {kDefaultWaitTime, NULL, false, false, false, kNoColorPrompt};
    parseArguments(&param, argc, argv);

    if (param.workDir != NULL) {
        if (chdir(param.workDir) != 0) {
            printLastError("chdir() failed");
            return errno;
        }
    }

#ifdef USE_LIBGIT2
    ret = gitInitialize();
    if (ret != 0) {
        releaseResource(false);
        return ret;
    }
#endif  // defined(USE_LIBGIT2)

    char gitConfigPath[PATH_MAX];
    if (gitGetConfigPath(gitConfigPath, sizeof(gitConfigPath)) != 0) {
        releaseResource(false);
        return 0;
    }

    if (param.noCache) {
        char statBuf[kShmSize];
        ret = getGitStatString(statBuf, sizeof(statBuf));
        if (ret != 0) {
            releaseResource(false);
            return ret;
        }
        ret = printPs1(stdout, param.promptType, 0, statBuf);
        releaseResource(false);
        return ret;
    }

    IpcKey ipcKey;
    ret = createIpcKey(gitConfigPath, &ipcKey);
    if (ret != 0) {
        return ret;
    }

    if (param.doReflesh) {
        ret = deleteSharedMemory(&ipcKey);
        int ret2 = deleteSemaphore(&ipcKey);
        return ret == 0 ? ret : ret2;
    }

    ret = getOrCreateSemaphre(&ipcKey, &g_semKey);
    if (ret != 0) {
        return errno;
    }
    g_shmId = getOrCreateSharedMemory(&ipcKey, kShmSize);
    if (g_shmId == -1) {
        return errno;
    }

    if (param.doReadOnly) {
        return printPs1(stdout, param.promptType, g_shmId, NULL);
    }

    signal(SIGINT, sigHandler);
    signal(SIGHUP, sigHandler);
    signal(SIGABRT, sigHandler);

    if (tryWaitSemaphre(&g_semKey)) {
        g_shouldSemPost = true;

        onLockAquired(param.waitTime, param.promptType);

        releaseResource(true);
        g_shouldSemPost = false;
    } else {
        onLockNotAquired(param.promptType);
        releaseResource(false);
    }

    return 0;
}


/*!
 * @brief Parse argument
 * @param [out] pParam  Parameter struct.
 * @param [in] argc  A number of command-line arguments
 * @param [in,out] argv  Command line arguments
 */
static void parseArguments(CmdParam *pParam, int argc, char *argv[])
{
    int ret, optidx;
    static const struct option kOptions[] = {
        {"clean",    no_argument,       NULL, 'c'},
        {"help",     no_argument,       NULL, 'h'},
        {"no-cache", no_argument,       NULL, 'n'},
        {"read",     no_argument,       NULL, 'r'},
        {"version",  no_argument,       NULL, 'v'},
        {"wait",     required_argument, NULL, 'w'},
        {"no-color", no_argument,       NULL, '\x01'},
        {"zsh",      no_argument,       NULL, '\x02'},
        {"tmux",     no_argument,       NULL, '\x03'},
        {NULL, 0, NULL, '\0'}  /* must be filled with zero */
    };

    while ((ret = getopt_long(argc, argv, "cC:hrvw:\x01\x02\x03", kOptions, &optidx)) != -1) {
        switch (ret) {
            case 'c':  /* -c, --clean */
                pParam->doReflesh = true;
                break;
            case 'C':  /* -C */
                pParam->workDir = optarg;
                break;
            case 'h':  /* -h, --help */
                showUsage(stdout, argv[0]);
                exit(EXIT_SUCCESS);
            case 'n':  /* -n, --no-cache */
                pParam->noCache = true;
                break;
            case 'r':  /* -r, --read */
                pParam->doReadOnly = true;
                break;
            case 'v':  /* -v, --version */
                showVersion(stdout);
                exit(EXIT_SUCCESS);
            case 'w':  /* -w, --wait */
                if (tryParseInt(optarg, &pParam->waitTime) != 0) {
                    exit(errno);
                }
                break;
            case '\x01':  /* --no-color */
                pParam->promptType = kNoColorPrompt;
                break;
            case '\x02':  /* --zsh */
                pParam->promptType = kZshPrompt;
                break;
            case '\x03':  /* --tmux */
                pParam->promptType = kTmuxPrompt;
                break;
            case '?':  /* unknown option */
            default:
                showUsage(stderr, argv[0]);
                exit(EINVAL);
        }
    }
}


/*!
 * @brief Show usage of this program
 * @param [in,out] stream  File stream to output.
 * @param [in] progName  Name of this program
 */
static void showUsage(FILE *stream, const char *progName)
{
    fprintf(
        stream,
        "[Usage]\n"
        "  $ %s [options]\n\n"
        "[Options]\n"
        "  -c, --clean\n"
        "    Clean semaphore and shared memory.\n"
        "  -C WORKDIR\n"
        "    Change directory before execute git command.\n"
        "  -h, --help\n"
        "    Show help of this program.\n"
        "  -r, --read\n"
        "    Read and print text in shaderd memory.\n"
        "  -v, --version\n"
        "    Show version of this program.\n"
        "  -w TIME, --wait=TIME\n"
        "    Specify wait time in milliseconds.\n"
        "    Default 1000\n", progName);
}


/*!
 * @brief Show version of this program
 * @param [in,out] stream  File stream to output.
 */
static void showVersion(FILE *stream)
{
    fprintf(
        stream,
        "Version: " VERSION "\n"
        "Build date: " __DATE__ ", " __TIME__ "\n"
        "Compiled by: " AUTHOR "\n");
}


/*!
 * @brief Parse string as integer value.
 * @param [in] numstr  Number string.
 * @param [out] pOutVal  Parse result destinatio.
 * @return Zero if success, otherwise non-zero.
 */
static int tryParseInt(const char *numstr, int *pOutVal)
{
    long outVal;
    char *p;

    if (pOutVal != NULL) {
        *pOutVal = 0;
    }

    if (numstr == NULL) {
        return errno = EINVAL;
    }

    numstr = skipSpace(numstr);
    if (numstr[0] == '\0') {
        return errno = EINVAL;
    }

    errno = 0;
    outVal = strtol(numstr, &p, 10);
    if (errno == ERANGE && (outVal == LONG_MIN || outVal == LONG_MAX)) {
        return errno;  // pgr0539対応のため戻り値をチェック
    } else if (errno != 0) {
        return errno;
    }

    if (*skipSpace(p) != '\0'  ) {
        return errno = EINVAL;
    }

    if (sizeof(int) == sizeof(long)) {
        if (pOutVal != NULL) {
            *pOutVal = (int)outVal;
        }
        return 0;
    }

    if (outVal < (long)INT_MIN || (long)INT_MAX < outVal) {
        return errno = ERANGE;
    }

    if (pOutVal != NULL) {
        *pOutVal = (int)outVal;
    }

    return 0;
}


/*!
 * @brief Search next non whitespace character.
 * @param [in] str  Source string.
 * @return A pointer to no whitespace character.
 */
static const char *skipSpace(const char *str)
{
    for (; *str != '\0'; str++) {
        if (!isspace(*str)) {
            break;
        }
    }

    return str;
}


/*!
 * @brief An action when lock aquired.
 * @param [in] waitTime  Wait time in milliseconds.
 * @param [in] promptType  Prompt type.
 * @return Zero if success, otherwise non-zero.
 */
static int onLockAquired(int waitTime, PromptType promptType)
{
    pid_t pid = fork();
    if (pid == -1) {
        printLastError("fork() failed");
        return errno;
    }

    if (pid == 0) {
        // Child process.
        g_targetPid = getpid();
        fclose(stdout);
        fclose(stdin);

        // Create shared memory if not exists.
        int ret = writeSharedMemory(g_shmId, NULL, 0);
        if (ret != 0) {
            return ret;
        }

        char statBuf[16];
        ret = getGitStatString(statBuf, sizeof(statBuf));
        if (ret != 0) {
            return ret;
        }

        ret = writeSharedMemory(g_shmId, (unsigned char *)statBuf, strlen(statBuf) + 1);
        if (ret != 0) {
            return ret;
        }
    } else {
        // Parent process.
        g_targetPid = 0;

        for (int t = 0; t < waitTime; t += kPollCycle) {
            int status;
            if ((pid = waitpid(pid, &status, WNOHANG)) == -1) {
                printLastError("waitpid() failed");
                return errno;
            }

            if (pid != 0) {
                if (!WIFEXITED(status)) {
                    fprintf(stderr, "waitpid() is finished but it's status is [%d]\n", WEXITSTATUS(status));
                    return WEXITSTATUS(status);
                }
                break;
            }

            usleep(kPollCycle * 1000);
        }

        int ret = printPs1(stdout, promptType, g_shmId, NULL);
        if (ret != 0) {
            return ret;
        }
    }

    return 0;
}


/*!
 * @brief An action when lock not aquired.
 * @param [in] promptType  Prompt type.
 * @return Zero if success, otherwise non-zero.
 */
static int onLockNotAquired(PromptType promptType)
{
    return printPs1(stdout, promptType, g_shmId, NULL);
}


/*!
 * @brief Write specified data to shared memory.
 * @param [in] shmId  Shared memory ID.
 * @param [in] data  Data to write.
 * @param [in] dataLen  Data length.
 * @return Zero if success, otherwise non-zero.
 */
static int writeSharedMemory(int shmId, const unsigned char *data, size_t dataLen)
{
    if (data == NULL || dataLen == 0) {
        return 0;
    }

    char *pShm = (char *)attachSharedMemory(shmId, kShmSize);
    if (pShm == NULL) {
        return errno;
    }

    memcpy(pShm, data, dataLen);

    if (dettachSharedMemory(pShm, kShmSize) != 0) {
        return errno;
    }

    return 0;
}


/*!
 * @brief Write specified data to shared memory.
 * @param [in] shmId  Shared memory ID.
 * @param [out] data  Data to write.
 * @param [in] bufSize  Buffer size.
 * @return Zero if success, otherwise non-zero.
 */
static int readSharedMemory(int shmId, char *data, size_t bufSize)
{
    char *pShm = (char *)attachSharedMemory(shmId, kShmSize);
    if (pShm == NULL) {
        return errno;
    }

    memcpy(data, pShm, bufSize);

    if (dettachSharedMemory(pShm, kShmSize) != 0) {
        return errno;
    }

    return 0;
}


/*!
 * @brief Print PS1 string.
 * @param [in,out] stream  File stream to output.
 * @param [in] promptType  Prompt type.
 * @param [in] shmId  Shared memory ID.
 * @param [in] statstr  Status string.
 * @return Zero if success, otherwise non-zero.
 */
static int printPs1(FILE *stream, PromptType promptType, int shmId, const char *statstr)
{
    char name[kNameBufSize];
    NameType nameType = getDisplayName(name, sizeof(name));
    if (name[0] == '\0') {
        return 1;
    }

    char statBuf[kShmSize];
    if (statstr == NULL) {
        readSharedMemory(shmId, statBuf, sizeof(statBuf));
        statstr = statBuf;
    }

    switch (promptType) {
        case kDefaultPrompt:
            printDefaultPs1(stream, nameType, name, statstr);
            break;
        case kZshPrompt:
            printZshPs1(stream, nameType, name, statstr);
            break;
        case kTmuxPrompt:
            printTmuxPs1(stream, nameType, name, statstr);
            break;
        case kNoColorPrompt:
        default:
            fprintf(stream, " %s", name);
            break;
    }

    return 0;
}


/*!
 * @brief Print PS1 string decorated with ANSI escape sequence.
 * @param [in,out] stream  File stream to output.
 * @param [in] nameType  Name type.
 * @param [in] name  Branch name, tag name or short commit hash.
 * @param [in] statstr  Status string.
 */
static void printDefaultPs1(FILE *stream, NameType nameType, const char *name, const char *statstr)
{
    const char *colorStartStr = "";
    const char *colorResetStr = "";
    const char *modifyStartStr = "";
    const char *modifyResetStr = "";

    switch (statstr[0]) {
        case '*':
            if (statstr[1] == '+') {
                colorStartStr = "\x1b[35m";
            } else {
                colorStartStr = "\x1b[31m";
            }
            break;
        case '+':
            colorStartStr = "\x1b[34m";
            break;
        case '%':
            colorStartStr = "\x1b[33m";
            break;
        default:
            colorStartStr = "\x1b[32m";
            break;
    }

    colorResetStr = "\x1b[0m";
    switch (nameType) {
        case kTag:
            modifyStartStr = "\x1b[4m";
            break;
        case kCommitHash:
#ifdef USE_ITALIC
            modifyStartStr = "\x1b[3m";
#else
            modifyStartStr = "\x1b[7m";
#endif  // defined(USE_ITALIC)
            break;
        case kNone:
        case kBranch:
        default:
            break;
    }

    fprintf(stream, " %s%s%s%s%s", colorStartStr, modifyStartStr, name, modifyResetStr, colorResetStr);
}


/*!
 * @brief Print PS1 string for zsh prompt.
 * @param [in,out] stream  File stream to output.
 * @param [in] nameType  Name type.
 * @param [in] name  Branch name, tag name or short commit hash.
 * @param [in] statstr  Status string.
 */
static void printZshPs1(FILE *stream, NameType nameType, const char *name, const char *statstr)
{
    const char *colorStartStr = "";
    const char *colorResetStr = "";
    const char *modifyStartStr = "";
    const char *modifyResetStr = "";

    switch (statstr[0]) {
        case '*':
            if (statstr[1] == '+') {
                colorStartStr = "%F{magenta}";
            } else {
                colorStartStr = "%F{red}";
            }
            break;
        case '+':
            colorStartStr = "%F{blue}";
            break;
        case '%':
            colorStartStr = "%F{yellow}";
            break;
        default:
            colorStartStr = "%F{green}";
            break;
    }

    colorResetStr = "%f";
    switch (nameType) {
        case kTag:
            modifyStartStr = "%U";
            modifyResetStr = "%u";
            break;
        case kCommitHash:
#ifdef USE_ITALIC
            modifyStartStr = "%{\x1b[3m%}";
            modifyResetStr = "%{\x1b[23m%}";
#else
            modifyStartStr = "%S";
            modifyResetStr = "%s";
#endif  // defined(USE_ITALIC)
            break;
        case kNone:
        case kBranch:
        default:
            break;
    }

    fprintf(stream, " %s%s%s%s%s", colorStartStr, modifyStartStr, name, modifyResetStr, colorResetStr);
}


/*!
 * @brief Print PS1 string for tmux status line.
 * @param [in,out] stream  File stream to output.
 * @param [in] nameType  Name type.
 * @param [in] name  Branch name, tag name or short commit hash.
 * @param [in] statstr  Status string.
 */
static void printTmuxPs1(FILE *stream, NameType nameType, const char *name, const char *statstr)
{
    const char *colorStartStr = "";
    const char *colorResetStr = "";
    const char *modifyStartStr = "";
    const char *modifyResetStr = "";

    switch (statstr[0]) {
        case '*':
            if (statstr[1] == '+') {
                colorStartStr = "#[fg=magenta]";
            } else {
                colorStartStr = "#[fg=red]";
            }
            break;
        case '+':
            colorStartStr = "#[fg=blue]";
            break;
        case '%':
            colorStartStr = "#[fg=yellow]";
            break;
        default:
            colorStartStr = "#[fg=green]";
            break;
    }
    colorResetStr = "#[fg=default]";
    switch (nameType) {
        case kTag:
            modifyStartStr = "#[underscore]";
            modifyResetStr = "#[default]";
            break;
        case kCommitHash:
#ifdef USE_ITALIC
            modifyStartStr = "#[italics]";
            modifyResetStr = "#[default]";
#else
            modifyStartStr = "#[reverse]";
            modifyResetStr = "#[default]";
#endif  // defined(USE_ITALIC)
            break;
        case kNone:
        case kBranch:
        default:
            break;
    }

    fprintf(stream, " %s%s%s%s%s", colorStartStr, modifyStartStr, name, modifyResetStr, colorResetStr);
}


/*!
 * @brief Signal handler.
 * @param [in] signum  Signal number.
 */
static void sigHandler(int signum)
{
    releaseResource(g_shouldSemPost);
    exit(signum);
}


/*!
 * @brief Release resources.
 * @param [in] shouldSemPost  true to call `sem_post()`.
 */
static void releaseResource(bool shouldSemPost)
{
#if USE_LIBGIT2
    gitUninitialize();
#endif  // defined(USE_LIBGIT2)

    if (g_targetPid != 0 && g_targetPid != getpid()) {
        return;
    }

    if (shouldSemPost) {
        postSemaphore(&g_semKey);
    }
}


/*!
 * @brief Get current branch name, tag name or short commit hash.
 * @param [out] buf  Buffer to write display name.
 * @param [in] bufSize  Buffer size.
 * @return Zero if success, otherwise non-zero.
 */
static NameType getDisplayName(char *buf, size_t bufSize)
{
    int ret = gitGetCurrentBranchName(buf, bufSize);
    if (ret != 0) {
        return kNone;
    }
    if (buf[0] != '\0') {
        return kBranch;
    }

    ret = gitGetCurrentTagName(buf, bufSize);
    if (buf[0] != '\0') {
        return kTag;
    }

    ret = gitGetCommitHashShort(buf, bufSize);
    if (ret != 0) {
        return kNone;
    }

    return kCommitHash;
}


/*!
 * @brief Get git status string.
 * @param [out] buf  Buffer to write status string.
 * @param [in] bufSize  Buffer size.
 * @return Zero if success, otherwise non-zero.
 */
static int getGitStatString(char *buf, size_t bufSize)
{
    if (bufSize < 4) {
        return 1;
    }

#ifdef USE_MULTI_THREAD
    int ret;

    pthread_t th1;
    bool isChanged;
    ret = pthread_create(&th1, NULL, thProcGitCheckChanged, &isChanged);
    if (ret != 0) {
        printLastError("pthread_create() failed");
        return errno;
    }

    pthread_t th2;
    bool isStaged;
    ret = pthread_create(&th2, NULL, thProcGitCheckStaged, &isStaged);
    if (ret != 0) {
        printLastError("pthread_create() failed");
        pthread_join(th1, NULL);
        return errno;
    }

    pthread_t th3;
    bool isUntracked;
    ret = pthread_create(&th3, NULL, thProcGitCheckUntracked, &isUntracked);
    if (ret != 0) {
        printLastError("pthread_create() failed");
        pthread_join(th2, NULL);
        pthread_join(th1, NULL);
        return errno;
    }

    pthread_join(th3, NULL);
    pthread_join(th2, NULL);
    pthread_join(th1, NULL);

    int i = 0;
    if (isChanged) {
        buf[i] = '*';
        i++;
    }

    if (isStaged) {
        buf[i] = '+';
        i++;
    }

    if (isUntracked) {
        buf[i] = '%';
        i++;
    }

    buf[i] = '\0';
#else
    int i = 0;
    bool isChanged = false;

#    ifdef DEBUG_PERFORMANCE_CHECK
    struct timespec start, end;
    long long elapsed;
    clock_gettime(CLOCK_MONOTONIC, &start);
#    endif  // defined(DEBUG_PERFORMANCE_CHECK)

    if (gitCheckChanged(&isChanged) == 0 && isChanged) {
        buf[i] = '*';
        i++;
    }

#    ifdef DEBUG_PERFORMANCE_CHECK
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) * 1000LL + (end.tv_nsec - start.tv_nsec) / 1000000LL;
    fprintf(stderr, "gitCheckChanged(): %lld ms\n", elapsed);

    clock_gettime(CLOCK_MONOTONIC, &start);
#    endif  // defined(DEBUG_PERFORMANCE_CHECK)

    if (gitCheckStaged(&isChanged) == 0 && isChanged) {
        buf[i] = '+';
        i++;
    }

#    ifdef DEBUG_PERFORMANCE_CHECK
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) * 1000LL + (end.tv_nsec - start.tv_nsec) / 1000000LL;
    fprintf(stderr, "gitCheckChanged(): %lld ms\n", elapsed);

    clock_gettime(CLOCK_MONOTONIC, &start);
#    endif  // defined(DEBUG_PERFORMANCE_CHECK)

    if (gitCheckUntracked(&isChanged) == 0 && isChanged) {
        buf[i] = '%';
        i++;
    }

#    ifdef DEBUG_PERFORMANCE_CHECK
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) * 1000LL + (end.tv_nsec - start.tv_nsec) / 1000000LL;
    fprintf(stderr, "gitCheckChanged(): %lld ms\n", elapsed);
#    endif  // defined(DEBUG_PERFORMANCE_CHECK)

    buf[i] = '\0';
#endif  // defined(USE_MULTI_THREAD)

    return 0;
}


#ifdef USE_MULTI_THREAD
/*!
 * @brief Thread process to call `gitCheckChanged()`.
 * @param [out] args  Output address for check result.
 * @return NULL.
 */
static void *thProcGitCheckChanged(void *args)
{
    bool isChanged;

#    ifdef DEBUG_PERFORMANCE_CHECK
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
#    endif  // defined(DEBUG_PERFORMANCE_CHECK)

    *(bool *)args = gitCheckChanged(&isChanged) == 0 && isChanged;

#    ifdef DEBUG_PERFORMANCE_CHECK
    clock_gettime(CLOCK_MONOTONIC, &end);
    long long elapsed = (end.tv_sec - start.tv_sec) * 1000LL + (end.tv_nsec - start.tv_nsec) / 1000000LL;
    fprintf(stderr, "gitCheckChanged(): %lld ms\n", elapsed);
#    endif  // defined(DEBUG_PERFORMANCE_CHECK)

    return NULL;
}


/*!
 * @brief Thread process to call `gitCheckStaged()`.
 * @param [out] args  Output address for check result.
 * @return NULL.
 */
static void *thProcGitCheckStaged(void *args)
{
    bool isStaged;

#    ifdef DEBUG_PERFORMANCE_CHECK
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
#    endif  // defined(DEBUG_PERFORMANCE_CHECK)

    *(bool *)args = gitCheckStaged(&isStaged) == 0 && isStaged;

#    ifdef DEBUG_PERFORMANCE_CHECK
    clock_gettime(CLOCK_MONOTONIC, &end);
    long long elapsed = (end.tv_sec - start.tv_sec) * 1000LL + (end.tv_nsec - start.tv_nsec) / 1000000LL;
    fprintf(stderr, "gitCheckStaged(): %lld ms\n", elapsed);
#    endif  // defined(DEBUG_PERFORMANCE_CHECK)

    return NULL;
}


/*!
 * @brief Thread process to call `gitCheckUntracked()`.
 * @param [out] args  Output address for check result.
 * @return NULL.
 */
static void *thProcGitCheckUntracked(void *args)
{
    bool isUntracked;

#    ifdef DEBUG_PERFORMANCE_CHECK
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
#    endif  // defined(DEBUG_PERFORMANCE_CHECK)

    *(bool *)args = gitCheckUntracked(&isUntracked) == 0 && isUntracked;

#    ifdef DEBUG_PERFORMANCE_CHECK
    clock_gettime(CLOCK_MONOTONIC, &end);
    long long elapsed = (end.tv_sec - start.tv_sec) * 1000LL + (end.tv_nsec - start.tv_nsec) / 1000000LL;
    fprintf(stderr, "gitCheckUntracked(): %lld ms\n", elapsed);
#    endif  // defined(DEBUG_PERFORMANCE_CHECK)

    return NULL;
}
#endif  // defined(USE_MULTI_THREAD)
