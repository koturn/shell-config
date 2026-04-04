/*!
 * @brief Logging utilities.
 * @author koturn
 * @file logging.c
 */
#include "logging.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>


/*!
 * @brief Print last error.
 * @param [in] msg  A Message.
 */
void printLastError(const char *msg)
{
    fprintf(stderr, "%s: errno=[%d][%s]\n", msg, errno, strerror(errno));
}
