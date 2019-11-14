#include <stdio.h>      // printf, fprintf, snprintf
#include <string.h>     // strerror
#include "common.h"

/**
 * Writes an error string to stderr with an informative error source, then returns the error code.
 * Used to abort from main loop or from a mock function.
 *
 * @param err The errno whose strerror() value we are printing to stderr
 * @param src The source of the error, printed alongside the error text
 * @return The errno value passed as first parameter
 */
int abort_with_errno(int err, const char *src)
{
    write_errno(err, src);
    return err;
}

/**
 * Writes a message to stderr and returns a non-zero value.
 * Used to abort from main loop or from a mock function.
 *
 * @param msg The message to print to stderr
 * @return 1
 */
int abort_with_message(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    return 1;
}

/**
 * Handle a read/write result deemed to be "bad" (likely because it does not match the expected length).
 * Writes an error message and returns a non-zero value. Used to abort from a mock function.
 *
 * @param rw_result The read/write result deemed to be "bad"
 * @param src The source of the error, printed alongside the error message
 * @return 1
 */
int handle_bad_rw_result(ssize_t rw_result, const char *src)
{
    // if errno is non-zero, there was an error in a system call
    if (errno != 0) return abort_with_errno(errno, "read 1");

    // declare and zero-out errmsg string
    char errmsg[RW_ERR_LEN];
    bzero(errmsg, RW_ERR_LEN);

    // put an error message into the string
    snprintf(errmsg, RW_ERR_LEN - 1, "error in %s: read %ld/%lu bytes", src, (long) rw_result,
             (unsigned long) sizeof(mytcp_t));

    // if we read zero bytes, we got an unexpected EOF
    if (rw_result == 0)
        snprintf(errmsg + strlen(errmsg), RW_ERR_LEN - strlen(errmsg), " (did the other side bail out?)");

    // print to stderr and return a non-zero exit code
    return abort_with_message(errmsg);
}
