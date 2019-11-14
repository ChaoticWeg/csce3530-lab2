#ifndef CSCE3530_LAB3_COMMON_H
#define CSCE3530_LAB3_COMMON_H

#include <errno.h>
#include <stdio.h>
#include "mytcp.h"

// help text macros
#define HELP_OPEN  "- Simulate opening a TCP connection"
#define HELP_CLOSE "- Simulate closing a TCP connection"

// error-handling related macros
#define RW_ERR_LEN 2048
#define write_errno(e, c) fprintf(stderr, "Error: %s (%s)\n", strerror((e)), (c))

// connectivity related macros
#define SERVER_HOSTNAME "cse01.cse.unt.edu"
#define SERVER_PORT 27015
#define CLIENT_PORT 27015

// return these from main() or mock_ACTION() to abort with a non-zero exit code
int abort_with_errno(int, const char *);
int abort_with_message(const char *);
int handle_bad_rw_result(ssize_t, const char *);

#endif //CSCE3530_LAB3_COMMON_H
