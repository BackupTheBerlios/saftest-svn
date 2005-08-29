#ifndef SAFTEST_DRIVER_H
#define SAFTEST_DRIVER_H

/**********************************************************************
 *
 *  I N C L U D E S
 *
 **********************************************************************/

#include "saftest_common.h"
#include "saftest_comm.h"
#include "saAis.h"

/**********************************************************************
 *
 *  D E F I N E S
 *
 **********************************************************************/

struct saftest_map_table_entry;
typedef struct saftest_map_table_entry {
    const char *request_op;
    const char *reply_op;
    SaAisErrorT (*client_handler)(
        int fd, 
        saftest_msg_t *request);
    void (*daemon_handler)(
        struct saftest_map_table_entry *map_entry,
        saftest_msg_t *request,
        saftest_msg_t **reply);
} saftest_map_table_entry_t;

typedef SaAisErrorT (*saftest_client_handler_func_t)(
    int fd, 
    saftest_msg_t *request);

typedef void (*saftest_daemon_message_handler_func_t)(
    struct saftest_map_table_entry *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply);

typedef void (*saftest_driver_init_func_t)(
    FILE *daemon_log_fp);

typedef void (*saftest_driver_add_fds_func_t)(
    int *max_fd,
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set);

typedef void (*saftest_driver_check_fds_func_t)(
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set);

typedef saftest_map_table_entry_t *
        (*saftest_get_map_table_func_t)();

/*extern saftest_client_handler_func_t saftest_client_generic_handle_request;*/

    /* 
     * I tried defining saftest_client_generic_handle_request in another file 
     * (with an extern declaration but couldn't get the compiler to accept it, 
     * and I just gave up.
     */
#define SAFTEST_MAP_TABLE_BEGIN() \
    SaAisErrorT \
    saftest_client_generic_handle_request( \
        int fd, \
        saftest_msg_t *request) \
    { \
        saftest_msg_t *reply; \
        SaAisErrorT status; \
        saftest_send_request(fd, get_library_id(), request, &reply); \
        if (NULL == reply) { \
            saftest_abort("Received no reply from the daemon\n"); \
        } \
        status = saftest_reply_msg_get_status(reply); \
        free(reply); \
        return(status); \
    } \
    saftest_map_table_entry_t msg_map_table[] = {

#define SAFTEST_MAP_TABLE_ENTRY(REQ, REP, CLIENT_HANDLER, DAEMON_HANDLER) \
    {REQ, REP, CLIENT_HANDLER, DAEMON_HANDLER},

#define SAFTEST_MAP_TABLE_END() \
    {0, 0, 0, 0}}; \
    saftest_map_table_entry_t * \
    saftest_driver_get_map_table() \
    { \
        return &(msg_map_table[0]);\
    }

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
