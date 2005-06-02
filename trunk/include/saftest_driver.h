#ifndef SAFTEST_DRIVER_H
#define SAFTEST_DRIVER_H

/**********************************************************************
 *
 *  I N C L U D E S
 *
 **********************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <assert.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <elf.h>
#include <dlfcn.h>
#include "glib.h"
#include "saAis.h"

/**********************************************************************
 *
 *  D E F I N E S
 *
 **********************************************************************/
#define BUF_SIZE 2048
#define RECV_RETRIES 5

#define AIS_B_VERSION_MAJOR 0x01
#define AIS_B_VERSION_MINOR 0x01
#define AIS_B_RELEASE_CODE 'B'

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

typedef struct fd_set_key {
    fd_set *set;
    int largest_fd;
} fd_set_key_t;

/**********************************************************************
 *
 *  G L O B A L S
 *
 **********************************************************************/

extern char    *optarg;
extern int      optind;

extern FILE *daemon_log_fp;
extern int   daemon_listen_fd;
extern char  daemon_pid_file[BUF_SIZE];

extern void 
saftest_driver_client_init(const char *run_path);

extern void
ais_test_log(const char *format, ...);

extern void
ais_test_abort(const char *format, ...);

extern void
err_exit(const char *format, ...);

extern const char *
get_error_string(SaAisErrorT error);

extern void
ais_test_uds_listen(int *fd, const char *socket_file);

extern void
ais_test_uds_accept(int fd, int *new_fd);

extern void
ais_test_uds_connect(int *fd, const char *socket_path);

extern void *
ais_test_recv_request(int client_connection_fd, int expected_length);

extern void *
ais_test_send_request(int fd, void *request,
                      int request_length, int expected_reply_length);

extern void
ais_test_send_reply(int client_connection_fd, void *reply, int reply_length);

extern void block_signals();

extern void unblock_signals();

extern void clear_pid_file(const char *pid_file);

extern void saftest_driver_init(const char *run_path);

extern void daemonize(int want_fork, const char *pid_file);

extern void setup_log_file(const char *log_file);

#endif /* SAFTEST_DRIVER_H */
