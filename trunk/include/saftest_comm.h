#ifndef SAFTEST_COMM_H
#define SAFTEST_COMM_H

/**********************************************************************
 *
 *  I N C L U D E S
 *
 **********************************************************************/

#include "saftest_common.h"
#include "saftest_list.h"
#include "saftest_system.h"
#include "os_saftest_comm.h"

#define SAFTEST_STRING_LENGTH 1024

typedef struct saftest_key_value_pair {
    char key[SAFTEST_STRING_LENGTH + 1];
    char value[SAFTEST_STRING_LENGTH + 1];
} saftest_key_value_pair_t;

/*
 * The internal and external key value pairs are lists of
 * saftest_key_value_t structures.  The internal ones are only used by
 * the saftest layer itself, and the external ones are utilized by
 * users of the saftest API.
 */
struct saftest_msg;

typedef struct saftest_msg {
    saftest_list internal_key_value_pairs;
    saftest_list external_key_value_pairs;
    struct saftest_msg *original_request;
} saftest_msg_t;

extern saftest_msg_t *
saftest_request_msg_create(const char *type);

/* !!! Remove this */
#define SAFTEST_MSG_DESTINATION_LIBRARY "LIB"

extern saftest_msg_t *
saftest_reply_msg_create(saftest_msg_t *original_msg,
                         const char *type, ubit32 status);

extern void
saftest_msg_set_str_value(saftest_msg_t *msg,
                          const char *key, const char *value);
extern void
saftest_msg_set_ubit8_value(saftest_msg_t *msg,
                            const char *key, ubit8 value);
extern void
saftest_msg_set_ubit16_value(saftest_msg_t *msg,
                             const char *key, ubit16 value);
extern void
saftest_msg_set_ubit32_value(saftest_msg_t *msg,
                             const char *key, ubit32 value);
extern void
saftest_msg_set_ubit64_value(saftest_msg_t *msg,
                             const char *key, ubit64 value);
extern const char *
saftest_msg_get_msg_type(saftest_msg_t *msg);

extern ubit32
saftest_reply_msg_get_status(saftest_msg_t *msg);

extern ubit32
saftest_msg_has_key(saftest_msg_t *msg, const char *key);

extern char *
saftest_msg_get_str_value(saftest_msg_t *msg, const char *key);

extern ubit8
saftest_msg_get_ubit8_value(saftest_msg_t *msg, const char *key);

extern ubit16
saftest_msg_get_ubit16_value(saftest_msg_t *msg, const char *key);

extern ubit32
saftest_msg_get_ubit32_value(saftest_msg_t *msg, const char *key);

extern sbit64
saftest_msg_get_sbit64_value(saftest_msg_t *msg, const char *key);

extern ubit64
saftest_msg_get_ubit64_value(saftest_msg_t *msg, const char *key);

extern const char *
saftest_msg_get_destination_library_id(saftest_msg_t *msg);

extern void saftest_msg_free(saftest_msg_t **msg);

extern void
saftest_uds_listen(int *fd, const char *socket_file);

extern void
saftest_uds_accept(int fd, int *new_fd);

extern void
saftest_uds_connect(int *fd, const char *socket_path);

extern void
saftest_uds_connect(int *fd, const char *socket_path);

extern void
saftest_send_request(int fd, const char *destination, const char *library_id,
                     saftest_msg_t *request, saftest_msg_t **reply);

extern void
saftest_send_reply(int client_connection_fd, saftest_msg_t *reply);

extern saftest_msg_t *
saftest_recv_request(int client_connection_fd);

extern saftest_msg_t *
saftest_recv_reply(int fd);

#endif /* SAFTEST_COMM_H */
