#include "saftest_common.h"
#include "saftest_comm.h"
#include "saftest_log.h"
#include "saftest_stdarg.h"

#define SAFTEST_KVP_INTERNAL 0x1
#define SAFTEST_KVP_EXTERNAL 0x2

#define SAFTEST_MSG_TYPE_KEY "SAFTEST_MSG_TYPE"
#define SAFTEST_REPLY_STATUS_KEY "SAFTEST_REPLY_STATUS"
#define SAFTEST_MSG_DESTINATION_KEY "SAFTEST_MSG_DESTINATION"
#define SAFTEST_LIBRARY_ID_KEY "SAFTEST_LIBRARY_ID"

#define SAFTEST_MSG_MAX_KEY_VALUE_PAIRS 50

typedef struct saftest_flat_msg {
    ubit32 num_internal_kvps;
    ubit32 num_external_kvps;
    saftest_key_value_pair_t internal_kvp_array[SAFTEST_MSG_MAX_KEY_VALUE_PAIRS];
    saftest_key_value_pair_t external_kvp_array[SAFTEST_MSG_MAX_KEY_VALUE_PAIRS];
} saftest_flat_msg_t;

ubit32
saftest_msg_get_internal_ubit32_value(saftest_msg_t *msg, const char *key);
/*
static ubit64
saftest_msg_get_internal_ubit64_value(saftest_msg_t *msg, const char *key);
*/
const char *
saftest_msg_get_internal_str_value(saftest_msg_t *msg, const char *key);
static void
saftest_msg_set_internal_ubit32_value(saftest_msg_t *msg,
                                      const char *key, ubit32 value);
/*
static void
saftest_msg_set_internal_ubit64_value(saftest_msg_t *msg,
                                      const char *key, ubit64 value);
*/
static void
saftest_msg_set_internal_str_value(saftest_msg_t *msg,
                                   const char *key, const char *value);
static void
saftest_msg_set_str_value_private(saftest_msg_t *msg,
                                  const char *key, const char *value,
                                  int flags);

static saftest_msg_t *
saftest_request_msg_create_private()
{
    saftest_msg_t *msg;

    msg = (saftest_msg_t *)malloc(sizeof(saftest_msg_t));
    if (NULL != msg) {
        memset(msg, 0, sizeof(saftest_msg_t));
    }
    msg->internal_key_value_pairs = saftest_list_create();
    msg->external_key_value_pairs = saftest_list_create();
    return(msg);
}

saftest_msg_t *
saftest_request_msg_create(const char *type)
{
    saftest_msg_t *msg;

    msg = saftest_request_msg_create_private();
    if (NULL != msg) {
        saftest_msg_set_internal_str_value(msg, SAFTEST_MSG_TYPE_KEY, type);
    }

    return(msg);
}

void
saftest_msg_free_kvp(void *data)
{
    saftest_key_value_pair_t *kvp = NULL;

    if (NULL == data) {
        return;
    }
    kvp = (saftest_key_value_pair_t *)data;
    free(kvp);
}

void
saftest_msg_free(saftest_msg_t **msg)
{

    if ((NULL != msg) && (NULL != *msg)) {
        saftest_list_delete_deep(&(*msg)->internal_key_value_pairs,
                                 saftest_msg_free_kvp);
        saftest_list_delete_deep(&(*msg)->external_key_value_pairs,
                                 saftest_msg_free_kvp);
        if (NULL != (*msg)->original_request) {
            saftest_msg_free(&((*msg)->original_request));
        }
        free(*msg);
        *msg = NULL;
    }
}

saftest_msg_t *
saftest_reply_msg_create(saftest_msg_t *original_msg,
                         const char *type, ubit32 status)
{
    saftest_msg_t *reply;

    /*
     * Here the original sender is now the destination, and vice-versa.
     */
    reply = saftest_request_msg_create(type);
    reply->original_request = original_msg;
    if (NULL != reply) {
        saftest_msg_set_internal_ubit32_value(
            reply, SAFTEST_REPLY_STATUS_KEY, status);
    }
    return(reply);
}

static saftest_key_value_pair_t *
saftest_msg_find_key_value_pair(saftest_msg_t *msg,
                                const char *key, int flags)
{
    saftest_key_value_pair_t *kvp = NULL;
    saftest_list list;
    saftest_list_element tmp_element;

    if (flags & SAFTEST_KVP_EXTERNAL) {
        list = msg->external_key_value_pairs;
    } else if (flags & SAFTEST_KVP_INTERNAL) {
        list = msg->internal_key_value_pairs;
    } else {
        saftest_abort("Must specify either internal or external\n");
    }

    tmp_element = saftest_list_first(list);
    while (NULL != tmp_element) {
        kvp = (saftest_key_value_pair_t *)tmp_element->data;
        if (0 == strcmp(kvp->key, key)) {
            break;
        }
        tmp_element = saftest_list_next(tmp_element);
        kvp = NULL;
    }
    return(kvp);
}

static void
saftest_msg_set_str_value_private(saftest_msg_t *msg,
                                  const char *key, const char *value, int flags)
{
    saftest_key_value_pair_t *kvp;
    saftest_list list;

    if (flags & SAFTEST_KVP_EXTERNAL) {
        list = msg->external_key_value_pairs;
    } else if (flags & SAFTEST_KVP_INTERNAL) {
        list = msg->internal_key_value_pairs;
    } else {
        saftest_abort("Must specify either internal or external\n");
    }

    assert(saftest_list_size(list) < SAFTEST_MSG_MAX_KEY_VALUE_PAIRS);
    kvp = saftest_msg_find_key_value_pair(msg, key, flags);
    if (NULL == kvp) {
        kvp = malloc(sizeof(saftest_key_value_pair_t));
        assert(NULL != kvp);
        memset(kvp, 0, sizeof(saftest_key_value_pair_t));
        saftest_list_element_create(list, kvp);
        assert(NULL != list);
        strncpy(kvp->key, key, SAFTEST_STRING_LENGTH);
    }
    strncpy(kvp->value, value, SAFTEST_STRING_LENGTH);
}

static void
saftest_msg_set_internal_ubit32_value(saftest_msg_t *msg,
                                      const char *key, ubit32 value)
{
    char ascii_value[SAFTEST_STRING_LENGTH + 1];

    memset(ascii_value, 0, sizeof(ascii_value));

    sprintf(ascii_value, "%d", value);
    saftest_msg_set_internal_str_value(msg, key, ascii_value);
}

/*
static void
saftest_msg_set_internal_ubit64_value(saftest_msg_t *msg,
                                            const char *key, ubit64 value)
{
    char ascii_value[SAFTEST_STRING_LENGTH + 1];

    memset(ascii_value, 0, sizeof(ascii_value));

    sprintf(ascii_value, "%lld", value);
    saftest_msg_set_internal_str_value(msg, key, ascii_value);
}
*/

void
saftest_msg_set_ubit8_value(saftest_msg_t *msg, const char *key, ubit8 value)
{
    char ascii_value[SAFTEST_STRING_LENGTH + 1];

    memset(ascii_value, 0, sizeof(ascii_value));

    sprintf(ascii_value, "%d", value);
    saftest_msg_set_str_value(msg, key, ascii_value);
}

void
saftest_msg_set_ubit16_value(saftest_msg_t *msg, const char *key, ubit16 value)
{
    char ascii_value[SAFTEST_STRING_LENGTH + 1];

    memset(ascii_value, 0, sizeof(ascii_value));

    sprintf(ascii_value, "%d", value);
    saftest_msg_set_str_value(msg, key, ascii_value);
}

void
saftest_msg_set_ubit32_value(saftest_msg_t *msg, const char *key, ubit32 value)
{
    char ascii_value[SAFTEST_STRING_LENGTH + 1];

    memset(ascii_value, 0, sizeof(ascii_value));

    sprintf(ascii_value, "%d", value);
    saftest_msg_set_str_value(msg, key, ascii_value);
}

void
saftest_msg_set_ubit64_value(saftest_msg_t *msg, const char *key, ubit64 value)
{
    char ascii_value[SAFTEST_STRING_LENGTH + 1];

    memset(ascii_value, 0, sizeof(ascii_value));

    sprintf(ascii_value, "%lld", value);
    saftest_msg_set_str_value(msg, key, ascii_value);
}

static void
saftest_msg_set_internal_str_value(saftest_msg_t *msg,
                                   const char *key, const char *value)
{
    saftest_msg_set_str_value_private(msg, key, value, SAFTEST_KVP_INTERNAL);
}

void
saftest_msg_set_str_value(saftest_msg_t *msg, const char *key, 
                          const char *value)
{
    saftest_msg_set_str_value_private(msg, key, value, SAFTEST_KVP_EXTERNAL);
}

const char *
saftest_msg_get_msg_type(saftest_msg_t *msg)
{
    const char *msg_type = NULL;

    msg_type = saftest_msg_get_internal_str_value(msg, SAFTEST_MSG_TYPE_KEY);
    assert(NULL != msg_type);
    return(msg_type);
}

ubit32
saftest_reply_msg_get_status(saftest_msg_t *msg)
{
    return saftest_msg_get_internal_ubit32_value(msg, SAFTEST_REPLY_STATUS_KEY);
}

ubit32
saftest_msg_get_internal_ubit32_value(saftest_msg_t *msg, const char *key)
{
    saftest_key_value_pair_t *kvp;
    ubit32 value = 0;

    kvp = saftest_msg_find_key_value_pair(msg, key, SAFTEST_KVP_INTERNAL);
    assert(NULL != kvp);

    value = atoi(kvp->value);
    return(value);
}

/*
ubit64
saftest_msg_get_internal_ubit64_value(saftest_msg_t *msg, const char *key)
{
    saftest_key_value_pair_t *kvp;
    ubit64 value = 0;

    kvp = saftest_msg_find_key_value_pair(msg, key, SAFTEST_KVP_INTERNAL);
    assert(NULL != kvp);

    value = SAFTEST_STRTOULL(kvp->value, NULL, 0);
    return(value);
}
*/

const char *
saftest_msg_get_internal_str_value(saftest_msg_t *msg, const char *key)
{
    saftest_key_value_pair_t *kvp;

    kvp = saftest_msg_find_key_value_pair(msg, key, SAFTEST_KVP_INTERNAL);
    if (NULL != kvp) {
        return(kvp->value);
    }
    return(NULL);
}

ubit8
saftest_msg_get_ubit8_value(saftest_msg_t *msg, const char *key)
{
    saftest_key_value_pair_t *kvp;
    ubit32 big_value = 0;
    ubit8 small_value = 0;

    kvp = saftest_msg_find_key_value_pair(msg, key, SAFTEST_KVP_EXTERNAL);
    assert(NULL != kvp);

    big_value = atoi(kvp->value);
    assert(big_value <= UBIT8_MAX);
    small_value = (ubit8) big_value;
    
    return(small_value);
}

ubit16
saftest_msg_get_ubit16_value(saftest_msg_t *msg, const char *key)
{
    saftest_key_value_pair_t *kvp;
    ubit32 big_value = 0;
    ubit16 small_value = 0;

    kvp = saftest_msg_find_key_value_pair(msg, key, SAFTEST_KVP_EXTERNAL);
    assert(NULL != kvp);

    big_value = atoi(kvp->value);
    assert(big_value <= UBIT16_MAX);
    small_value = (ubit16) big_value;
    
    return(small_value);
}

ubit32
saftest_msg_get_ubit32_value(saftest_msg_t *msg, const char *key)
{
    saftest_key_value_pair_t *kvp;
    ubit32 value = 0;

    kvp = saftest_msg_find_key_value_pair(msg, key, SAFTEST_KVP_EXTERNAL);
    assert(NULL != kvp);

    value = atoi(kvp->value);
    return(value);
}

sbit64
saftest_msg_get_sbit64_value(saftest_msg_t *msg, const char *key)
{
    saftest_key_value_pair_t *kvp;
    sbit64 value = 0;

    kvp = saftest_msg_find_key_value_pair(msg, key, SAFTEST_KVP_EXTERNAL);
    assert(NULL != kvp);

    value = SAFTEST_STRTOLL(kvp->value, NULL, 0);
    return(value);
}

ubit64
saftest_msg_get_ubit64_value(saftest_msg_t *msg, const char *key)
{
    saftest_key_value_pair_t *kvp;
    ubit64 value = 0;

    kvp = saftest_msg_find_key_value_pair(msg, key, SAFTEST_KVP_EXTERNAL);
    assert(NULL != kvp);

    value = SAFTEST_STRTOULL(kvp->value, NULL, 0);
    return(value);
}

ubit32
saftest_msg_has_key(saftest_msg_t *msg, const char *key)
{
    saftest_key_value_pair_t *kvp;

    kvp = saftest_msg_find_key_value_pair(msg, key, SAFTEST_KVP_EXTERNAL);
    if (NULL != kvp) {
        return(1);
    }
    return(0);
}

char *
saftest_msg_get_str_value(saftest_msg_t *msg, const char *key)
{
    saftest_key_value_pair_t *kvp;

    kvp = saftest_msg_find_key_value_pair(msg, key, SAFTEST_KVP_EXTERNAL);
    if (NULL != kvp) {
        return(kvp->value);
    }
    return(NULL);
}

void
saftest_flat_msg_add_internal_kvp(void *data, void *key)
{
    saftest_flat_msg_t *sfm;
    saftest_key_value_pair_t *kvp = NULL;

    if (NULL == data) {
        return;
    }
    kvp = (saftest_key_value_pair_t *)data;
    sfm = (saftest_flat_msg_t *)key;
    memcpy(&(sfm->internal_kvp_array[sfm->num_internal_kvps]), kvp,
           sizeof(saftest_key_value_pair_t));
    sfm->num_internal_kvps++;
}

void
saftest_flat_msg_add_external_kvp(void *data, void *key)
{
    saftest_flat_msg_t *sfm;
    saftest_key_value_pair_t *kvp = NULL;

    if (NULL == data) {
        return;
    }
    kvp = (saftest_key_value_pair_t *)data;
    sfm = (saftest_flat_msg_t *)key;
    memcpy(&(sfm->external_kvp_array[sfm->num_external_kvps]), kvp,
           sizeof(saftest_key_value_pair_t));
    sfm->num_external_kvps++;
}

saftest_flat_msg_t *
saftest_create_flat_msg_from_saftest_msg(saftest_msg_t *msg)
{
    saftest_flat_msg_t *sfm = NULL;

    sfm = (saftest_flat_msg_t *)malloc(sizeof(saftest_flat_msg_t));
    assert(NULL != sfm);
    memset(sfm, 0, sizeof(saftest_flat_msg_t));

    saftest_list_each(msg->internal_key_value_pairs, 
                      saftest_flat_msg_add_internal_kvp, sfm);
    saftest_list_each(msg->external_key_value_pairs, 
                      saftest_flat_msg_add_external_kvp, sfm);
    return(sfm);
}

saftest_msg_t *
saftest_create_saftest_msg_from_flat_msg(saftest_flat_msg_t *sfm)
{
    saftest_msg_t *msg;
    char tmp_key[SAFTEST_STRING_LENGTH + 1];
    char tmp_value[SAFTEST_STRING_LENGTH + 1];
    ubit32 kvp_ndx = 0;

    msg = saftest_request_msg_create_private();
    assert(NULL != msg);

    for (kvp_ndx = 0; kvp_ndx < sfm->num_internal_kvps; kvp_ndx++) {
        memcpy(tmp_key, sfm->internal_kvp_array[kvp_ndx].key, sizeof(tmp_key));
        memcpy(tmp_value, 
               sfm->internal_kvp_array[kvp_ndx].value, sizeof(tmp_value));
        saftest_msg_set_internal_str_value(msg, tmp_key, tmp_value);
    } 
    for (kvp_ndx = 0; kvp_ndx < sfm->num_external_kvps; kvp_ndx++) {
        memcpy(tmp_key, sfm->external_kvp_array[kvp_ndx].key, sizeof(tmp_key));
        memcpy(tmp_value, 
               sfm->external_kvp_array[kvp_ndx].value, sizeof(tmp_value));
        saftest_msg_set_str_value(msg, tmp_key, tmp_value);
    } 

    return(msg);
}

void
saftest_uds_listen(int *fd, const char *socket_file)
{
    int optval = 1;
    int ret;
    struct sockaddr_un  sun;

    saftest_log("saftest_uds_listen on %s\n", socket_file);
    memset(&sun, 0, sizeof(sun));

    *fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == *fd) {
        err_exit("Unable to open a socket: %s\n", strerror(errno));
    }

    /*
     * Set the close-on-exec flag so that processes that are forked
     * off don't keep the file descriptor open.
     */
    ret = fcntl(*fd, F_SETFD, 1);
    if (-1 == ret) {
        close(*fd);
        err_exit("fcntl failed: %s\n", strerror(errno));
    }

    ret = setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (-1 == ret) {
        close(*fd);
        err_exit("setsockopt SO_REUSEADDR failed: %s\n", strerror(errno));
    }

    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, socket_file);
    ret = bind(*fd, (struct sockaddr *)&sun, sizeof(sun));
    if (0 != ret) {
        /*
         * The socket file exists.  Check if there is a live process
         * bound to it.  If not, remove the file and try to bind again.
         */
        if (connect(*fd, (struct sockaddr *)&sun, sizeof(sun)) == 0) {
            close(*fd);
            err_exit("Failed to bind to %s: %s\n",
                     socket_file, strerror(errno));
        }
        else {
            unlink(socket_file);
            if (bind(*fd, (struct sockaddr *)&sun, sizeof(sun)) != 0) {
                close(*fd);
                err_exit("Failed to bind to %s: %s\n",
                        socket_file, strerror(errno));
            }
        }

    }

    ret = chmod(socket_file, S_IRUSR | S_IWUSR);
    if (0 != ret) {
        close(*fd);
        unlink(socket_file);
        err_exit("Failed to chmod %s: %s\n",
                 socket_file, strerror(errno));
    }

    if (listen(*fd, SOMAXCONN) != 0) {
        close(*fd);
        unlink(socket_file);
        err_exit("Failed to listen on %s: %s\n",
                 socket_file, strerror(errno));
    }
}

void
saftest_uds_accept(int fd, int *new_fd)
{
    int                ret;
    int                optval = 1;

    *new_fd = accept(fd, NULL, 0);
    if (-1 == *new_fd) {
        saftest_log("Failed to accept on fd %d: %s\n",
                     fd, strerror(errno));
    }

    /*
     * Turn on keep-alive so that we can detect when the remote
     * goes away while we are waiting for a reply message
     */
    do {
        ret = setsockopt(*new_fd, SOL_SOCKET, SO_KEEPALIVE,
                         &optval, sizeof(optval));
        if (-1 == ret && ret != EINTR) {
            close(*new_fd);
            err_exit("setsockopt KEEPALIVE failed: %s\n", strerror(errno));
        }
    } while (-1 == ret);
}

void
saftest_uds_connect(int *fd, const char *socket_path)
{
    struct sockaddr_un  sun;
    int                ret;
    int                optval = 1;

    memset(&sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, socket_path);

    *fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == *fd) {
        err_exit("Unable to open a socket: %s\n", strerror(errno));
    }

    ret = connect(*fd, (struct sockaddr *)&sun, sizeof(sun));
    if (-1 == ret) {
        close(*fd);
        err_exit("Unable to connect to %s: %s\n",
                 socket_path, strerror(errno));
    }

    do {
        ret = setsockopt(*fd, SOL_SOCKET, SO_KEEPALIVE,
                         &optval, sizeof(optval));
        if (-1 == ret && ret != EINTR) {
            close(*fd);
            err_exit("setsockopt KEEPALIVE failed: %s\n", strerror(errno));
        }
    } while (-1 == ret);
}

const char *
saftest_msg_get_destination_library_id(saftest_msg_t *msg)
{
    return(saftest_msg_get_internal_str_value(msg, SAFTEST_LIBRARY_ID_KEY));
}

static void
saftest_msg_set_destination_library_id(saftest_msg_t *msg,
                                       const char *library_id)
{
    saftest_msg_set_internal_str_value(msg, SAFTEST_LIBRARY_ID_KEY,
                                       library_id);
}

void
saftest_send_request(int fd, const char *destination, const char *library_id,
                     saftest_msg_t *request, saftest_msg_t **reply)
{
    int bytes_sent;
    saftest_flat_msg_t *sfm = NULL;

    if (NULL != library_id) {
        saftest_msg_set_destination_library_id(request, library_id);
    }

    sfm = saftest_create_flat_msg_from_saftest_msg(request);
    assert(NULL != sfm);

    bytes_sent = send(fd, sfm, sizeof(saftest_flat_msg_t), 0);
    free(sfm);
    if (bytes_sent < sizeof(saftest_flat_msg_t)) {
        saftest_abort("Error sending on socket %d: "
                       "Expected %d bytes, sent %d bytes\n",
                       fd, sizeof(saftest_flat_msg_t), bytes_sent);
    }
    if (NULL != reply) {
        (*reply) = saftest_recv_reply(fd);
    }
}

void
saftest_send_reply(int client_connection_fd, saftest_msg_t *reply)
{
    int bytes_sent = 1;
    int bytes_left;
    int total_length = sizeof(saftest_flat_msg_t);
    /* int counter; */
    saftest_flat_msg_t *sfm = NULL;

    assert(NULL != reply);
    
            saftest_msg_set_destination_library_id(
            reply,
            saftest_msg_get_destination_library_id(reply->original_request));

    sfm = saftest_create_flat_msg_from_saftest_msg(reply);
    assert(NULL != sfm);

    for (bytes_left = total_length;
         (bytes_left > 0) && (bytes_sent > 0);
         bytes_left -= bytes_sent)  {
        bytes_sent = send(client_connection_fd, 
                          (char *)sfm + (total_length - bytes_left),
                          bytes_left, 0);
        if (-1 == bytes_sent) {
            saftest_abort("Error sending on socket %d: %s\n",
                          client_connection_fd, 
                          strerror(errno));
        }
    }
    if (bytes_left > 0) {
        saftest_abort("Error sending on socket %d: Only sent %d bytes of %d\n",
                      client_connection_fd, 
                      total_length - bytes_left,
                      total_length);
    }
    free(sfm);
    saftest_msg_free(&reply);
}

static saftest_flat_msg_t *
saftest_recv_flat_msg(int fd)
{
    saftest_flat_msg_t *sfm = NULL;
    int expected_length = sizeof(saftest_flat_msg_t);
    int bytes_recvd = 1;
    int bytes_left;
    /* int counter = 0; */

    sfm = malloc(expected_length);
    assert(NULL != sfm);

    for (bytes_left = expected_length;
         (bytes_left > 0) && (bytes_recvd > 0);
         bytes_left -= bytes_recvd) {
        bytes_recvd = recv(fd, 
                           (char *)sfm + (expected_length - bytes_left), 
                           bytes_left, SAFTEST_RECV_FLAG);
/*
        saftest_log("recv_flat counter: %d bytes_recvd: %d bytes_left: %d\n",
                    counter, bytes_recvd, bytes_left);
*/
    }
    if (bytes_left > 0) {
/*
        saftest_log("Error receiving on socket %d: "
                    "Expected %d bytes, received %d bytes\n",
                    fd,
                    expected_length,
                    expected_length - bytes_left);
*/
        free(sfm);
        sfm = NULL;
    }
    return(sfm);
}

saftest_msg_t *
saftest_recv_request(int fd)
{
    saftest_msg_t *request = NULL;
    saftest_flat_msg_t *sfm = NULL;

    sfm = saftest_recv_flat_msg(fd);
    if (NULL == sfm) {
        request = NULL;
    } else {
        request = saftest_create_saftest_msg_from_flat_msg(sfm);
        assert(NULL != request);
        free(sfm);
    }
    return(request);
}

saftest_msg_t *
saftest_recv_reply(int fd)
{
    saftest_msg_t *reply = NULL;
    saftest_flat_msg_t *sfm = NULL;

    sfm = saftest_recv_flat_msg(fd);
    if (NULL == sfm) {
        reply = NULL;
    } else {
        reply = saftest_create_saftest_msg_from_flat_msg(sfm);
        assert(NULL != reply);
        free(sfm);
    }
    return(reply);
}
