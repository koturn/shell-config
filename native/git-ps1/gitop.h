/*!
 * @brief Git operation utilities.
 * @author koturn
 * @file gitop.h
 */
#ifndef GITOP_H
#define GITOP_H


#include <stdbool.h>
#include <stddef.h>

#ifdef USE_LIBGIT2
#include <git2.h>
#endif  // defined(USE_LIBGIT2)


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */


#if !defined(USE_LIBGIT2) && defined(__GNUC__)
__attribute__((const))
int gitInitialize(void);
__attribute__((const))
int gitUninitialize(void);
#else
int gitInitialize(void);
int gitUninitialize(void);
#endif  // !defined(USE_LIBGIT2) && defined(__GNUC__)
int gitGetConfigPath(char *gitConfigPath, size_t bufSize);
int gitGetCurrentBranchName(char *buf, size_t bufSize);
int gitGetCurrentTagName(char *buf, size_t bufSize);
int gitGetCommitHashShort(char *buf, size_t bufSize);
int gitCheckChanged(bool *pIsChanged);
int gitCheckStaged(bool *pIsStaged);
int gitCheckUntracked(bool *pIsUntracked);


#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */


#endif  /* GITOP_H */
