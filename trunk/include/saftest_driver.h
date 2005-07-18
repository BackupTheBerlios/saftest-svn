#ifndef SAFTEST_DRIVER_H
#define SAFTEST_DRIVER_H

/**********************************************************************
 *
 *  I N C L U D E S
 *
 **********************************************************************/

#include "saftest_common.h"

/**********************************************************************
 *
 *  D E F I N E S
 *
 **********************************************************************/

typedef void (*saftest_driver_init_func_t)(
    FILE *daemon_log_fp);

typedef void (*saftest_driver_handle_client_message_func_t)(
    int client_connection_fd,
    void *message);

typedef void (*saftest_driver_add_fds_func_t)(
    int *max_fd,
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set);

typedef void (*saftest_driver_check_fds_func_t)(
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set);

typedef int (*saftest_driver_client_main_func_t)(
    int argc, char *argv[], void *first_request, int first_request_length);

/**********************************************************************
 *
 *  G L O B A L S
 *
 **********************************************************************/

extern void 
saftest_driver_client_init(const char *run_path);

extern void block_signals();

extern void unblock_signals();

extern void clear_pid_file(const char *pid_file);

extern void saftest_driver_init(const char *run_path);

extern void daemonize(int want_fork, const char *pid_file);

extern FILE *setup_log_file(const char *log_file);

#endif /* SAFTEST_DRIVER_H */
