/*!
 * @brief Git operation utilities.
 * @author koturn
 * @file gitop.c
 */
#ifndef _DEFAULT_SOURCE
#    define _DEFAULT_SOURCE
#endif  // !defined(_DEFAULT_SOURCE)

#include "gitop.h"
#include "logging.h"

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>
#include <unistd.h>

#ifdef USE_LIBGIT2
#include <git2.h>
#endif  // defined(USE_LIBGIT2)


#ifdef USE_LIBGIT2
static git_repository *g_pRepo = NULL;
#else
//! Git comamnd to get top directory of git.
static const char kCmdGitTopLevel[] = "git rev-parse --show-toplevel";
//! Git comamnd to get current branch name.
static const char kCmdGitGetCurrentBranchName[] = "git branch --show-current 2> /dev/null";
//! Git comamnd to get current tag name.
static const char kCmdGitGetCurrentTagName[] = "git tag --points-at HEAD 2> /dev/null";
//! Git comamnd to get current commit hash.
static const char kCmdGitGetCurrentCommitHashShort[] = "git rev-parse --short HEAD 2> /dev/null";
//! Git comamnd to check whether changed file exists or not.
static const char kCmdGitCheckChanged[] = "git diff --no-ext-diff --quiet";
//! Git comamnd to check whether staged file exists or not.
static const char kCmdGitCheckStaged[] = "git diff --no-ext-diff --cached --quiet";
//! Git comamnd to check whether untracked file exists or not.
static const char kCmdGitCheckUntracked[] = "git ls-files --others --exclude-standard --directory --no-empty-directory --error-unmatch -- ':/*' >/dev/null 2>/dev/null";
#endif  // defined(USE_LIBGIT2)


static bool isDirectory(const char *path);
#ifdef USE_LIBGIT2
static bool scanDir(git_index *pIndex, const char *base, const char *relpath);
static bool isTracked(git_index *pIndex, const char *path);
static int gitDiffCallback(const git_diff_delta *delta, float progress, void *payload);
static int gitUntrackedCallback(const char *path, unsigned int statFlags, void *payload);
#else
static int execCommandReadFirstLine(const char *command, char *buf, size_t bufSize);
#endif  // USE_LIBGIT2


/*!
 * @brief Initialize libgit2 and get repository object pointer.
 *
 * This function has no effect is `USE_LIBGIT2` is not defined.
 *
 * @return Zero if success, otherwise non-zero.
 */
int gitInitialize(void)
{
#ifdef USE_LIBGIT2
    int ret = git_libgit2_init();
    if (ret < 0) {
        const git_error *e = git_error_last();
        fprintf(stderr, "%s failed [%d]; %s\n", "git_libgit2_init()", ret, e != NULL && e->message != NULL ? e->message : "unknown");
        return ret;
    }

    ret = git_repository_open_ext(&g_pRepo, ".", GIT_REPOSITORY_OPEN_NO_SEARCH == 0 ? GIT_REPOSITORY_OPEN_CROSS_FS : 0, NULL);
    if (ret != 0) {
        // const git_error *e = git_error_last();
        // fprintf(stderr, "%s failed [%d]; %s\n", "git_repository_open_ext()", ret, e != NULL && e->message != NULL ? e->message : "unknown");
        return ret;
    }
#endif  // defined(USE_LIBGIT2)

    return 0;
}


/*!
 * @brief Free resouces of libgit2.
 *
 * This function has no effect is `USE_LIBGIT2` is not defined.
 *
 * @return Zero if success, otherwise non-zero.
 */
int gitUninitialize(void)
{
#ifdef USE_LIBGIT2
    if (g_pRepo != NULL) {
        git_repository_free(g_pRepo);
        g_pRepo = NULL;
    }
    int ret = git_libgit2_shutdown();
    if (ret < 0) {
        const git_error *e = git_error_last();
        fprintf(stderr, "%s failed [%d]; %s\n", "git_libgit2_shutdown()", ret, e != NULL && e->message != NULL ? e->message : "unknown");
        return ret;
    }
#endif  // defined(USE_LIBGIT2)

    return 0;
}


/*!
 * @brief Get repository configuration file path.
 * @param [out] buf  Buffer to write git configuration path.
 * @param [in] bufSize  Buffer size.
 * @return Zero if success, otherwise non-zero.
 */
int gitGetConfigPath(char *buf, size_t bufSize)
{
#if USE_LIBGIT2
    int ret = git_repository_open_ext(&g_pRepo, ".", GIT_REPOSITORY_OPEN_NO_SEARCH == 0 ? GIT_REPOSITORY_OPEN_CROSS_FS : 0, NULL);
    if (ret != 0) {
        const git_error *e = git_error_last();
        fprintf(stderr, "Error opening repository: %s\n", e != NULL && e->message != NULL ? e->message : "unknown");
        return ret;
    }

    const char *gitTopDir = git_repository_workdir(g_pRepo);
    if (gitTopDir == NULL) {
        fprintf(stderr, "This is a bare repository (no working directory)\n");
        return 1;
    }

    char dotGitPath[PATH_MAX];
    int iChars = snprintf(dotGitPath, sizeof(dotGitPath), "%s.git", gitTopDir);
    if (iChars >= (int)sizeof(dotGitPath)) {
        fprintf(stderr, "Path too long\n");
        return 2;
    }
#else
    char gitTopDir[1024];
    int ret = execCommandReadFirstLine(kCmdGitTopLevel, gitTopDir, sizeof(gitTopDir));
    if (ret != 0) {
        return ret;
    }

    char dotGitPath[PATH_MAX];
    int iChars = snprintf(dotGitPath, sizeof(dotGitPath), "%s/.git", gitTopDir);
    if (iChars >= (int)sizeof(dotGitPath)) {
        fprintf(stderr, "Path too long\n");
        return 2;
    }
#endif  // defined(USE_LIBGIT2)

    if (isDirectory(dotGitPath)) {
        // Normal repository
        iChars = snprintf(buf, bufSize, "%s/config", dotGitPath);
    } else {
        // worktree
        iChars = snprintf(buf, bufSize, "%s", dotGitPath);
    }

    if (iChars >= (int)bufSize) {
        fprintf(stderr, "Path too long\n");
        return 2;
    }

    return 0;
}


/*!
 * @brief Get branch name at the HEAD.
 * @param [out] buf  Buffer to write branch name.
 * @param [in] bufSize  Buffer size.
 * @return Zero if success, otherwise non-zero.
 */
int gitGetCurrentBranchName(char *buf, size_t bufSize)
{
#if USE_LIBGIT2
    buf[0] = '\0';

    int ret;
    git_reference *pRefHead = NULL;
    if ((ret = git_repository_head(&pRefHead, g_pRepo)) < 0) {
        if (ret == GIT_EUNBORNBRANCH || ret == GIT_ENOTFOUND) {
            return 0;
        }
        return ret;
    }

    if (git_reference_is_branch(pRefHead)) {
        // branch
        const char *name = git_reference_shorthand(pRefHead);
        snprintf(buf, bufSize, "%s", name);
    }

    git_reference_free(pRefHead);
    return 0;
#else
    int ret = execCommandReadFirstLine(kCmdGitGetCurrentBranchName, buf, bufSize);
    if (ret != 0) {
        return ret;
    }

    return 0;
#endif  // defined(USE_LIBGIT2)
}


/*!
 * @brief Get tag name at the HEAD.
 * @param [out] buf  Buffer to write tag name.
 * @param [in] bufSize  Buffer size.
 * @return Zero if success, otherwise non-zero.
 */
int gitGetCurrentTagName(char *buf, size_t bufSize)
{
#if USE_LIBGIT2
    git_reference *pRefHead = NULL;
    git_object *pObjHead = NULL;
    git_reference_iterator *pRefIter = NULL;

    git_reference *pRef = NULL;
    const git_oid *pOid;

    buf[0] = '\0';

    int ret;
    if ((ret = git_repository_head(&pRefHead, g_pRepo)) < 0) {
        if (ret == GIT_EUNBORNBRANCH || ret == GIT_ENOTFOUND) {
            return 0;
        }
        return ret;
    }

    if ((ret = git_reference_peel(&pObjHead, pRefHead, GIT_OBJECT_COMMIT)) < 0) {
        goto cleanup;
    }

    if ((ret = git_reference_iterator_glob_new(&pRefIter, g_pRepo, "refs/tags/*")) < 0) {
        goto cleanup;
    }

    pOid = git_object_id(pObjHead);
    while ((ret = git_reference_next(&pRef, pRefIter)) == 0) {
        git_object *pObj = NULL;
        if (git_reference_peel(&pObj, pRef, GIT_OBJECT_COMMIT) == 0) {
            if (git_oid_equal(git_object_id(pObj), pOid)) {
                const char *tagName = git_reference_shorthand(pRef);
                snprintf(buf, bufSize, "%s", tagName);
                git_object_free(pObj);
                git_reference_free(pRef);
                ret = 0;
                goto cleanup;
            }
        }

        git_object_free(pObj);
        git_reference_free(pRef);
    }

    if (ret == GIT_ITEROVER) {
        ret = 1;
    }

cleanup:
    git_reference_iterator_free(pRefIter);
    git_object_free(pObjHead);
    git_reference_free(pRefHead);

    return ret;
#else
    int ret = execCommandReadFirstLine(kCmdGitGetCurrentBranchName, buf, bufSize);
    if (ret != 0 || buf[0] != '\0') {
        return ret;
    }

    ret = execCommandReadFirstLine(kCmdGitGetCurrentTagName, buf, bufSize);
    if (ret != 0 || buf[0] != '\0') {
        return ret;
    }

    return 0;
#endif  // defined(USE_LIBGIT2)
}


/*!
 * @brief Get git short commit hash.
 * @param [out] buf  Buffer to write branch or tag name.
 * @param [in] bufSize  Buffer size.
 * @return Zero if success, otherwise non-zero.
 */
int gitGetCommitHashShort(char *buf, size_t bufSize)
{
#if USE_LIBGIT2
    git_object *pObj = NULL;
    git_buf gitBuf = GIT_BUF_INIT;

    buf[0] = '\0';

    int ret;
    if ((ret = git_revparse_single(&pObj, g_pRepo, "HEAD")) < 0) {
        return ret;
    }

    if ((ret = git_object_short_id(&gitBuf, pObj)) < 0) {
        goto cleanup;
    }

    snprintf(buf, bufSize, "%s", gitBuf.ptr);

cleanup:
    git_buf_dispose(&gitBuf);
    git_object_free(pObj);
    return ret;
#else
    return execCommandReadFirstLine(kCmdGitGetCurrentCommitHashShort, buf, bufSize);
#endif  // defined(USE_LIBGIT2)
}


/*!
 * @brief Check whether changed file is exists or not.
 * @param [out] pIsChanged  Destination of checking result.
 * @return Zero if success, otherwise non-zero.
 */
int gitCheckChanged(bool *pIsChanged)
{
#if USE_LIBGIT2
    int ret;

    git_index *index = NULL;
    if ((ret = git_repository_index(&index, g_pRepo)) < 0) {
        return ret;
    }

    git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
    // opts.flags |= GIT_DIFF_INCLUDE_UNTRACKED;
    // opts.flags |= GIT_DIFF_RECURSE_UNTRACKED_DIRS;

    git_diff *diff = NULL;
    if ((ret = git_diff_index_to_workdir(&diff, g_pRepo, index, &opts)) < 0) {
        git_index_free(index);
        return ret;
    }

    ret = git_diff_foreach(diff, gitDiffCallback, NULL, NULL, NULL, NULL);

    git_diff_free(diff);
    git_index_free(index);

    if (ret == 1) {
        *pIsChanged = true;
        return 0;
    } else if (ret == 0) {
        *pIsChanged = false;
        return 0;
    } else {
        *pIsChanged = false;
        return ret;
    }
#else
    *pIsChanged = system(kCmdGitCheckChanged) != 0;
    return 0;
#endif  // defined(USE_LIBGIT2)
}


/*!
 * @brief Check whether staged file is exists or not.
 * @param [out] pIsStaged  Destination of checking result.
 * @return Zero if success, otherwise non-zero.
 */
int gitCheckStaged(bool *pIsStaged)
{
#if USE_LIBGIT2
    int ret;

    git_object *pHead = NULL;
    git_tree *pTree = NULL;
    git_index *pIndex = NULL;
    git_diff *pDiff = NULL;
    git_diff_options opts = GIT_DIFF_OPTIONS_INIT;

    if ((ret = git_revparse_single(&pHead, g_pRepo, "HEAD")) < 0) {
        return ret;
    }

    if ((ret = git_object_peel((git_object **)&pTree, pHead, GIT_OBJECT_TREE)) < 0) {
        goto cleanup;
    }

    if ((ret = git_repository_index(&pIndex, g_pRepo)) < 0) {
        goto cleanup;
    }

    if ((ret = git_diff_tree_to_index(&pDiff, g_pRepo, pTree, pIndex, &opts)) < 0) {
        goto cleanup;
    }

    ret = git_diff_foreach(pDiff, gitDiffCallback, NULL, NULL, NULL, NULL);

    if (ret == 1) {
        *pIsStaged = true;
        return 0;
    } else if (ret == 0) {
        *pIsStaged = false;
        return 0;
    } else {
        *pIsStaged = false;
        return ret;
    }

cleanup:
    git_diff_free(pDiff);
    git_index_free(pIndex);
    git_tree_free(pTree);
    git_object_free(pHead);

    return ret;
#else
    *pIsStaged = system(kCmdGitCheckStaged) != 0;
    return 0;
#endif  // defined(USE_LIBGIT2)
}


/*!
 * @brief Check whether untracked file is exists or not.
 * @param [out] pIsUntracked  Destination of checking result.
 * @return Zero if success, otherwise non-zero.
 */
int gitCheckUntracked(bool *pIsUntracked)
{
#if USE_LIBGIT2
    git_index *pIndex = NULL;

    int ret = git_repository_index(&pIndex, g_pRepo);
    if (ret < 0) {
        return ret;
    }

    const char *workdir = git_repository_workdir(g_pRepo);
    if (workdir == NULL) {
        git_index_free(pIndex);
        return -1;
    }

    *pIsUntracked = scanDir(pIndex, workdir, "");

    git_index_free(pIndex);

    return 0;
#else
    *pIsUntracked = system(kCmdGitCheckUntracked) == 0;
    return 0;
#endif  // defined(USE_LIBGIT2)
}


/*!
 * @brief Check whether specified path is directory or not.
 * @param [in] path  File path to check.
 * @return Zero if success, otherwise non-zero.
 */
static bool isDirectory(const char *path)
{
    struct stat st;

    int ret = stat(path, &st);
    if (ret != 0) {
        fprintf(stderr, "%s: errno=[%d][%s]\n", "stat() failed", errno, strerror(errno));
        return false;
    }

    return (st.st_mode & S_IFMT) == S_IFDIR;
}


#ifdef USE_LIBGIT2
/*!
 * @brief Find untracked file recursively.
 * @param [in] pIndex  Pointer to `git_index` object.
 * @param [in] base  Base directory path.
 * @param [in] relpath  Relative path from base directory.
 * @return True if untracked file found, otherwise false.
 */
static bool scanDir(git_index *pIndex, const char *base, const char *relpath)
{
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", base, relpath);

    DIR *pDir = opendir(path);
    if (pDir == NULL) {
        return false;
    }

    struct dirent *pEnt;
    while ((pEnt = readdir(pDir)) != NULL) {
        if (strcmp(pEnt->d_name, ".") == 0 || strcmp(pEnt->d_name, "..") == 0) {
            continue;
        }

        char childRel[PATH_MAX];
        if (relpath[0]) {
            snprintf(childRel, sizeof(childRel), "%s/%s", relpath, pEnt->d_name);
        } else {
            snprintf(childRel, sizeof(childRel), "%s", pEnt->d_name);
        }

        char child_full[PATH_MAX];
        snprintf(child_full, sizeof(child_full), "%s/%s", base, childRel);

        int ignored = 0;
        if (git_ignore_path_is_ignored(&ignored, g_pRepo, childRel) == 0 && ignored) {
            continue;
        }

        struct stat st;
        if (stat(child_full, &st) != 0) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            if (scanDir(pIndex, base, childRel)) {
                closedir(pDir);
                return true;
            }
        } else {
            if (!isTracked(pIndex, childRel)) {
                closedir(pDir);
                return true;
            }
        }
    }

    closedir(pDir);

    return false;
}


/*!
 * @brief Find untracked file recursively.
 * @param [in] pIndex  Pointer to `git_index` object.
 * @param [in] path  File path.
 * @return True if untracked file found, otherwise false.
 */
static bool isTracked(git_index *pIndex, const char *path)
{
    return git_index_get_bypath(pIndex, path, 0) != NULL;
}


/*!
 * @brief Callback function for `git_diff_foreach`.
 * @param [in] pDelta  Pointer to diff delta (unused).
 * @param [in] progress  Progress (unused).
 * @param [in] payload  Argument for this callback (unused).
 * @return Zero if success, otherwise non-zero.
 */
int gitDiffCallback(const git_diff_delta *pDelta, float progress, void *payload)
{
    (void)pDelta;
    (void)progress;
    (void)payload;
    return 1;
}


/*!
 * @brief Callback function for `git_status_foreach_ext`.
 * @param [in] path  Path to untracked file (unused).
 * @param [in] statFlags  Status flag bits.
 * @param [in] payload  Argument for this callback (unused).
 * @return Zero if success, otherwise non-zero.
 */
int gitUntrackedCallback(const char *path, unsigned int statFlags, void *payload)
{
    (void)path;
    (void)payload;

    if (statFlags & GIT_STATUS_WT_NEW) {
        return 1;  // Untracked file found.
    }

    return 0;
}
#else
/*!
 * @brief Execute command and read first-line from its output.
 * @param [in] command  A command string.
 * @param [out] buf  A buffer to store command output.
 * @param [in] bufSize  Buffer size.
 * @return 0 if success, otherwise false.
 */
static int execCommandReadFirstLine(const char *command, char *buf, size_t bufSize)
{
    buf[0] = '\0';

    FILE *pProc = popen(command, "r");
    if (pProc == NULL) {
        printLastError("popen() failed");
        return errno;
    }

    if (fgets(buf, (int)bufSize, pProc) != NULL) {
        if (buf[0] != '\0') {
            buf[strlen(buf) - 1] = '\0';
        }
    }

    return pclose(pProc);
}
#endif  // defined(USE_LIBGIT2)
