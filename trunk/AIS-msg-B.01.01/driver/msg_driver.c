/**********************************************************************
 *
 *	I N C L U D E S
 *
 **********************************************************************/
#include "saftest_driver_lib_utils.h"
#include "saftest_log.h"
#include "saftest_comm.h"
#include "saMsg.h"

/**********************************************************************
 *
 *	D E F I N E S
 *
 **********************************************************************/

/**********************************************************************
 *
 *	G L O B A L S
 *
 **********************************************************************/

enum ais_test_msg_request_op {
    AIS_TEST_MSG_REQUEST_INVALID=0,
    AIS_TEST_MSG_REQUEST_CREATE_TEST_RESOURCE,
    AIS_TEST_MSG_REQUEST_MSG_INITALIZE,
    AIS_TEST_MSG_REQUEST_MSG_SELECTION_OBJECT_GET,
    AIS_TEST_MSG_REQUEST_DISPATCH,
    AIS_TEST_MSG_REQUEST_MSG_FINALIZE,
    AIS_TEST_MSG_REQUEST_MSG_QUEUE_OPEN,
};
typedef enum ais_test_msg_request_op ais_test_msg_request_op_t;

/*
 * op - Used for all requests
 * requestor_pid - Used for all requests
 * release_code, major_version, minor_version - Used by MSG_INITIALIZE
 * queue_open_cb_flag - Used by MSG_INITIALIZE
 * queue_group_track_cb_flag - Used by MSG_INITIALIZE
 * message_delivered_cb_flag - Used by MSG_INITIALIZE
 * message_received_cb_flag - Used by MSG_INITIALIZE
 * msg_name - Used by RESOURCE_OPEN, RESOURCE_OPEN_ASYNC
 * msg_resource_id - Used by RESOURCE_OPEN, RESOURCE_OPEN_ASYNC,
 *                    RESOURCE_CLOSE, MSG_SYNC, MSG_ASYNC, UNMSG
 * msg_mode - Used by MSG_SYNC, MSG_ASYNC
 */

typedef struct ais_test_msg_request {
    ais_test_msg_request_op_t op;
    pid_t requestor_pid;
    SaVersionT sa_version;
    int msg_resource_id;
    int queue_open_cb_flag;
    int queue_group_track_cb_flag;
    int message_delivered_cb_flag;
    int message_received_cb_flag;
    SaDispatchFlagsT dispatch_flags;
    int null_msg_handle_flag;
    int null_callbacks_flag;
    int null_version_flag;
    int null_selection_object_flag;
    char queue_name[BUF_SIZE];
} ais_test_msg_request_t;

/*
 * status - always valid
 * msg_resource_id - Only valid when request op was 
 *                    AIS_TEST_MSG_REQUEST_MSG_INITALIZE
 */
typedef struct ais_test_msg_reply {
    SaAisErrorT status;
    int msg_resource_id;
} ais_test_msg_reply_t;

typedef struct msg_resource {
    int msg_resource_id;
    pthread_t thread_id;

    SaVersionT version;
    SaNameT queue_name;
    SaNameT queue_group_name;
    SaTimeT timeout;
    SaDispatchFlagsT dispatch_flags;
    SaMsgHandleT msg_handle;
    SaMsgCallbacksT msg_callbacks;
    SaSelectionObjectT selection_object;
} msg_resource_t;

GList *msg_list = NULL;

void
usage()
{
    printf("Server Usage: msg_driver --daemon\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --log-file <log file>\n");
    printf("                         --pid-file <pid file>\n");
    printf("                        [-no-daemonize]\n");

    printf("\n");

    printf("Client Usage: msg_driver --o CREATE_TEST_RES\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("\n");
    printf("Client Usage: msg_driver --o INIT\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --dispatch-flags \n");
    printf("                           <SA_MSG_DISPATCH_BLOCKING |\n");
    printf("                            SA_MSG_DISPATCH_ONE |\n");
    printf("                            SA_MSG_DISPATCH_ALL>  \n");
    printf("                        [--set-queue-open-cb]\n");
    printf("                        [--set-queue-group-track-cb]\n");
    printf("                        [--set-message-delivered-cb]\n");
    printf("                        [--set-message-received-cb]\n");
    printf("                        [--version-release-code <code #>]\n");
    printf("                        [--version-major <code #>]\n");
    printf("                        [--version-minor <code #>]\n");
    printf("                        [--null-msg-handle]\n");
    printf("                        [--null-callbacks]\n");
    printf("                        [--null-version]\n");
    printf("\n");
    printf("Client Usage: msg_driver --o SELECT_OBJ_GET\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                        [--null-selection-object]\n");
    printf("\n");
    printf("Client Usage: msg_driver --o RES_OPEN\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                         --msg-name <Lock Name>\n");
    printf("                        [--timeout <timeout>]\n");
    printf("\n");
    printf("Client Usage: msg_driver --o RES_OPEN_ASYNC\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                         --msg-name <Lock Name>\n");
    printf("\n");
    printf("Client Usage: msg_driver --o RES_CLOSE\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("\n");
    printf("Client Usage: msg_driver --o FINALIZE\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("\n");
    printf("Client Usage: msg_driver --o DISPATCH\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                         --dispatch-flags \n");
    printf("                           <SA_MSG_DISPATCH_BLOCKING |\n");
    printf("                            SA_MSG_DISPATCH_ONE |\n");
    printf("                            SA_MSG_DISPATCH_ALL>  \n");
    printf("\n");
	exit(255);
}

const char *get_library_id()
{
    return "MSG";
}

int get_library_message_size()
{
    return sizeof(ais_test_msg_request_t);
}

void saftest_daemon_init(FILE *log_fp)
{
    assert(NULL != log_fp);
    ais_test_log_set_fp(log_fp);
}

static ais_test_msg_request_op_t
ais_test_map_string_to_op(const char *str)
{
    if (0 == strcmp(str, "CREATE_TEST_RES")) {
        return AIS_TEST_MSG_REQUEST_CREATE_TEST_RESOURCE;
    } else if (0 == strcmp(str, "INIT")) {
        return AIS_TEST_MSG_REQUEST_MSG_INITALIZE;
    } else if (0 == strcmp(str, "SELECT_OBJ_GET")) {
        return AIS_TEST_MSG_REQUEST_MSG_SELECTION_OBJECT_GET;
    } else if (0 == strcmp(str, "DISPATCH")) {
        return AIS_TEST_MSG_REQUEST_DISPATCH;
    } else if (0 == strcmp(str, "FINALIZE")) {
        return AIS_TEST_MSG_REQUEST_MSG_FINALIZE;
    } else if (0 == strcmp(str, "QUEUE_OPEN")) {
        return AIS_TEST_MSG_REQUEST_MSG_QUEUE_OPEN;
    }
    ais_test_abort("Unknown op string %s\n", str);
    return AIS_TEST_MSG_REQUEST_INVALID;
}

int
get_next_msg_resource_id()
{
    static int next_msg_resource_id = 1;
    int ret_id;

    ret_id = next_msg_resource_id;
    next_msg_resource_id += 1;
    return(ret_id);
}

msg_resource_t *
add_msg_resource()
{
    msg_resource_t *res;

    res = malloc(sizeof(msg_resource_t));
    assert(NULL != res);
    memset(res, 0, sizeof(msg_resource_t));

    msg_list = g_list_append(msg_list, res);
    res->msg_resource_id = get_next_msg_resource_id();
    ais_test_log("Added a msg resource with id %d\n", res->msg_resource_id);
    return(res);
}

void
delete_msg_resource(msg_resource_t *res)
{
    ais_test_log("Deleting msg resource with id %d\n", res->msg_resource_id);
    msg_list = g_list_remove(msg_list, res);
    free(res);
}

msg_resource_t *
lookup_msg_resource(int msg_resource_id)
{
    GList *element;
    msg_resource_t *res;

    for (element = g_list_first(msg_list);
         NULL != element;
         element = g_list_next(element)) {
        res = (msg_resource_t *)element->data;
        if (res->msg_resource_id == msg_resource_id) {
            return(res);
        }
    }
    return(NULL);
}

void
ais_test_daemon_queue_open_callback(SaInvocationT invocation,
                           SaMsgQueueHandleT queueHandle,
                           SaAisErrorT error)
{
    ais_test_abort("Define me\n");
}

void
ais_test_daemon_queue_group_track_callback(
    const SaNameT *queueGroupName,
    const SaMsgQueueGroupNotificationBufferT *notificationBuffer,
    SaUint32T numberOfMembers,
    SaAisErrorT error)
{
    ais_test_abort("Define me\n");
}

void
ais_test_daemon_message_delivered_callback(SaInvocationT invocation,
                                           SaAisErrorT error)
{
    ais_test_abort("Define me\n");
}

void
ais_test_daemon_message_received_callback(SaInvocationT invocation)
{
    ais_test_abort("Define me\n");
}

static void *
ais_test_daemon_dispatch_thread(void *arg)
{
    msg_resource_t *msg_res = (msg_resource_t *)arg;
    SaAisErrorT err;
    
    err = saMsgDispatch(msg_res->msg_handle, SA_DISPATCH_BLOCKING);
    if (err != SA_AIS_OK) {
        ais_test_abort("Error %s calling "
                       "saMsgDispatch(SA_DISPATCH_BLOCKING)\n",
                       get_error_string(err));
    }
    return(NULL);
}

void 
ais_test_daemon_handle_create_test_res_request(
    ais_test_msg_request_t *request,
    ais_test_msg_reply_t *reply)
{
    msg_resource_t *msg_res;
    
    ais_test_log("Received a create test resource request from pid %d.\n",
                 request->requestor_pid);

    msg_res = add_msg_resource();

    reply->msg_resource_id = msg_res->msg_resource_id;
    reply->status = SA_AIS_OK;
}

void 
ais_test_daemon_handle_init_request(ais_test_msg_request_t *request,
                                    ais_test_msg_reply_t *reply)
{
    msg_resource_t *msg_res;
    int err;
    SaMsgHandleT *handle = NULL;
    SaMsgCallbacksT *callbacks = NULL;
    SaVersionT *version = NULL;
    
    ais_test_log("Received an init request from pid %d for id %d "
                 "release code=%d, majorVersion=%d, minorVersion=%d\n",
                 request->requestor_pid,
                 request->msg_resource_id,
                 request->sa_version.releaseCode,
                 request->sa_version.majorVersion,
                 request->sa_version.minorVersion);

    msg_res = lookup_msg_resource(request->msg_resource_id);
    if (NULL == msg_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->msg_resource_id);
    }

    if (0 == request->null_msg_handle_flag) {
        handle = &msg_res->msg_handle;
    }
    if (0 == request->null_callbacks_flag) {
        callbacks = &msg_res->msg_callbacks;
    }
    if (0 == request->null_version_flag) {
        version = &msg_res->version;
    }

    msg_res->msg_callbacks.saMsgQueueOpenCallback = NULL;
    msg_res->msg_callbacks.saMsgQueueGroupTrackCallback = NULL;
    msg_res->msg_callbacks.saMsgMessageDeliveredCallback = NULL;
    msg_res->msg_callbacks.saMsgMessageReceivedCallback = NULL;

    if (request->queue_open_cb_flag) {
        msg_res->msg_callbacks.saMsgQueueOpenCallback = 
            ais_test_daemon_queue_open_callback;
    }
    if (request->queue_group_track_cb_flag) {
        msg_res->msg_callbacks.saMsgQueueGroupTrackCallback = 
            ais_test_daemon_queue_group_track_callback;
    }
    if (request->message_delivered_cb_flag) {
        msg_res->msg_callbacks.saMsgMessageDeliveredCallback =
            ais_test_daemon_message_delivered_callback;
    }
    if (request->message_received_cb_flag) {
        msg_res->msg_callbacks.saMsgMessageReceivedCallback =
            ais_test_daemon_message_received_callback;
    }

    msg_res->version = request->sa_version;
    msg_res->dispatch_flags = request->dispatch_flags;

    ais_test_log("Before calling saMsgInitialize\n");
    reply->status = saMsgInitialize(handle, callbacks, version);
    ais_test_log("After calling saMsgInitialize\n");
    ais_test_log("status is %d\n", reply->status);
    if (SA_AIS_OK == reply->status) {
        if (request->dispatch_flags == SA_DISPATCH_BLOCKING) {
            ais_test_log("Starting new dispatch thread\n");
            err = pthread_create(&msg_res->thread_id, NULL, 
                                 ais_test_daemon_dispatch_thread,
                                 (void*)msg_res);
            if (err) {
                ais_test_abort("Error creating new thread: (%d) %s\n", 
                               errno, strerror(errno));
            }
        }
    }
}

void 
ais_test_daemon_handle_selection_object_request(ais_test_msg_request_t *request,
                                                ais_test_msg_reply_t *reply)
{
    msg_resource_t *msg_res = NULL;
    SaSelectionObjectT *selection_object = NULL;

    ais_test_log("Received a select object get request for id %d\n",
                 request->msg_resource_id);
    msg_res = lookup_msg_resource(request->msg_resource_id);
    if (0 == request->null_selection_object_flag) {
        selection_object = &msg_res->selection_object;
    }
    if (NULL == msg_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->msg_resource_id);
    }
    reply->status = saMsgSelectionObjectGet(msg_res->msg_handle, 
                                            selection_object);
    if (0 == request->null_selection_object_flag) {
        ais_test_log("New Lock Selection Object on fd %d\n",
                     selection_object);
    }
}

void 
ais_test_daemon_handle_dispatch_request(ais_test_msg_request_t *request,
                                        ais_test_msg_reply_t *reply)
{
    msg_resource_t *msg_res = NULL;

    ais_test_log("Received a dispatch request for id %d\n",
                 request->msg_resource_id);
    msg_res = lookup_msg_resource(request->msg_resource_id);
    if (NULL == msg_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->msg_resource_id);
    }
    reply->status = saMsgDispatch(msg_res->msg_handle, 
                                  request->dispatch_flags);
}

void 
ais_test_daemon_handle_finalize_request(ais_test_msg_request_t *request,
                                        ais_test_msg_reply_t *reply)
{
    msg_resource_t *msg_res = NULL;

    ais_test_log("Received a finalize request for id %d\n",
                 request->msg_resource_id);
    msg_res = lookup_msg_resource(request->msg_resource_id);
    if (NULL == msg_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->msg_resource_id);
    }
    reply->status = saMsgFinalize(msg_res->msg_handle);
    msg_res->selection_object = 0;
}

void 
ais_test_daemon_handle_queue_open_request(ais_test_msg_request_t *request,
                                          ais_test_msg_reply_t *reply)
{
    msg_resource_t *msg_res = NULL;

    ais_test_log("Received a queue open request for id %d queue name %s\n",
                request->msg_resource_id, request->queue_name);
    msg_res = lookup_msg_resource(request->msg_resource_id);
    if (NULL == msg_res) {
        ais_test_abort("Unknown resource id %d\n",
                    request->msg_resource_id);
    }
}

void
ais_test_daemon_handle_incoming_client_message(int client_connection_fd,
                                               void *void_request)
{
    ais_test_msg_request_t *request = (ais_test_msg_request_t *)void_request;
    ais_test_msg_reply_t *reply;

    if (NULL == request) {
        ais_test_abort("Invalid (NULL) request\n");
    }

    reply = (ais_test_msg_reply_t *)malloc(sizeof(ais_test_msg_reply_t));
    memset(reply, 0, sizeof(ais_test_msg_reply_t));

    switch(request->op) {
        case AIS_TEST_MSG_REQUEST_CREATE_TEST_RESOURCE:
            ais_test_daemon_handle_create_test_res_request(request, reply);
            break;
        case AIS_TEST_MSG_REQUEST_MSG_INITALIZE:
            ais_test_daemon_handle_init_request(request, reply);
            break;
        case AIS_TEST_MSG_REQUEST_MSG_SELECTION_OBJECT_GET:
            ais_test_daemon_handle_selection_object_request(request, reply);
            break;
        case AIS_TEST_MSG_REQUEST_DISPATCH:
            ais_test_daemon_handle_dispatch_request(request, reply);
            break;
        case AIS_TEST_MSG_REQUEST_MSG_FINALIZE:
            ais_test_daemon_handle_finalize_request(request, reply);
            break;
        case AIS_TEST_MSG_REQUEST_MSG_QUEUE_OPEN:
            ais_test_daemon_handle_queue_open_request(request, reply);
            break;
        default:
            ais_test_abort("Daemon received request with unknown op %d\n",
                           request->op);
    }
    free(request);

    ais_test_log("Before calling send_reply\n");
    ais_test_send_reply(client_connection_fd, reply,
                        sizeof(ais_test_msg_reply_t));
    ais_test_log("After calling send_reply\n");
    free(reply);
}

void
ais_test_daemon_handle_incoming_msg_message(gpointer data, gpointer user_data)
{
    msg_resource_t *msg_res = data;
    fd_set *fd_mask = (fd_set *)user_data;
    SaAisErrorT err;

    if (NULL == data) {
        return;
    }

    if (!FD_ISSET(msg_res->selection_object, fd_mask)) {
        return;
    }

    ais_test_log("Incoming request on msg selection fd %d\n",
                 msg_res->selection_object);
    err = saMsgDispatch(msg_res->msg_handle, msg_res->dispatch_flags);
    if (SA_AIS_OK != err) {
        ais_test_log("Error %s performing saMsgDispatch\n",
                     get_error_string(err));
    }
}

void ais_test_daemon_add_msg_resource_to_fdset(gpointer data, 
                                               gpointer user_data)
{
    msg_resource_t *msg_res;
    fd_set_key_t *set_key = (fd_set_key_t *)user_data;

    if (NULL == data) {
        return;
    }

    msg_res = (msg_resource_t *)data;
    if (msg_res->thread_id > 0) {
        /* It will have its own dispatch thread */
        return;
    }

    if (msg_res->selection_object > 0) {
        FD_SET(msg_res->selection_object, set_key->set);
        if (msg_res->selection_object > set_key->largest_fd) {
            set_key->largest_fd = msg_res->selection_object;
        }
    }
    return;
}

void ais_test_daemon_add_fds(
    int *max_fd,
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    fd_set_key_t set_key;

    set_key.set = read_fd_set;
    set_key.largest_fd = *max_fd;

    g_list_foreach(msg_list, ais_test_daemon_add_msg_resource_to_fdset,
                   &set_key);
    *max_fd = set_key.largest_fd;
}

void
ais_test_daemon_check_fds(
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    g_list_foreach(msg_list,
                   ais_test_daemon_handle_incoming_msg_message,
                   read_fd_set);
}

SaAisErrorT
ais_test_client_handle_create_test_res_request(
    int fd,
    ais_test_msg_request_t *request)
{
    ais_test_msg_reply_t *reply;
    SaAisErrorT status;
 
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_msg_request_t),
                                  sizeof(ais_test_msg_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    if (SA_AIS_OK == status) {
        ais_test_log("Resource ID=%d\n", reply->msg_resource_id);
    }
    free(reply);
    return(status);
}

SaAisErrorT
ais_test_client_handle_init_request(int fd,
                                    ais_test_msg_request_t *request,
                                    int msg_resource_id,
                                    SaVersionT *sa_version,
                                    int queue_open_cb_flag,
                                    int queue_group_track_cb_flag,
                                    int message_delivered_cb_flag,
                                    int message_received_cb_flag,
                                    SaDispatchFlagsT dispatch_flags,
                                    int null_msg_handle_flag,
                                    int null_callbacks_flag,
                                    int null_version_flag)
{
    ais_test_msg_reply_t *reply;
    SaAisErrorT status;

    request->msg_resource_id = msg_resource_id;
    request->sa_version = *sa_version;
    request->queue_open_cb_flag = queue_open_cb_flag;
    request->queue_group_track_cb_flag = queue_group_track_cb_flag;
    request->message_delivered_cb_flag = message_delivered_cb_flag;
    request->message_received_cb_flag = message_received_cb_flag;
    request->dispatch_flags = dispatch_flags;
    request->null_msg_handle_flag = null_msg_handle_flag;
    request->null_callbacks_flag = null_callbacks_flag;
    request->null_version_flag = null_version_flag;
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_msg_request_t),
                                  sizeof(ais_test_msg_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }
    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_selection_object_request(int fd,
                                             ais_test_msg_request_t *request,
                                             int msg_resource_id,
                                             int null_selection_object_flag)
{
    ais_test_msg_reply_t *reply;
    SaAisErrorT status;
 
    request->msg_resource_id = msg_resource_id;
    request->null_selection_object_flag = null_selection_object_flag;
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_msg_request_t),
                                  sizeof(ais_test_msg_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_dispatch_request(int fd,
                                        ais_test_msg_request_t *request,
                                        int msg_resource_id,
                                        SaDispatchFlagsT dispatch_flags)
{
    ais_test_msg_reply_t *reply;
    SaAisErrorT status;
 
    request->msg_resource_id = msg_resource_id;
    request->dispatch_flags = dispatch_flags;
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_msg_request_t),
                                  sizeof(ais_test_msg_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }
    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_resource_finalize_request(
    int fd,
    ais_test_msg_request_t *request, 
    int msg_resource_id)
{
    ais_test_msg_reply_t *reply;
    SaAisErrorT status;
 
    request->msg_resource_id = msg_resource_id;
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_msg_request_t),
                                  sizeof(ais_test_msg_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_queue_open_request(int fd,
                                          ais_test_msg_request_t *request,
                                          int msg_resource_id,
                                          const char *queue_name)
{
    ais_test_msg_reply_t *reply;
    SaAisErrorT status;
 
    request->msg_resource_id = msg_resource_id;
    strcpy(request->queue_name, queue_name);
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_msg_request_t),
                                  sizeof(ais_test_msg_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    free(reply);
    return(status);
}

#define HELP_OPTION 1
#define DAEMON_OPTION 2
#define NO_DAEMONIZE_OPTION 3
#define SOCKET_FILE_OPTION 4
#define RUN_DIR_OPTION 5
#define LOG_FILE_OPTION 6
#define PID_FILE_OPTION 7
#define OP_NAME_OPTION 8
#define QUEUE_NAME_OPTION 9
#define TIMEOUT_OPTION 10 
#define SET_QUEUE_OPEN_CB_OPTION 11
#define SET_QUEUE_GROUP_TRACK_CB_OPTION 12
#define SET_MESSAGE_DELIVERED_CB_OPTION 13
#define SET_MESSAGE_RECEIVED_CB_OPTION 14
#define MSG_RESOURCE_ID_OPTION 15
#define VERBOSE_OPTION 18
#define EXPECTED_MSG_STATUS_OPTION 19
#define DISPATCH_FLAGS_OPTION 20 
#define NULL_MSG_HANDLE_OPTION 24
#define NULL_CALLBACKS_OPTION 25
#define NULL_VERSION_OPTION 26
#define NULL_MSG_ID_OPTION 27
#define NULL_MSG_STATUS_OPTION 28
#define NULL_SELECTION_OBJECT_OPTION 29
#define INVOCATION_OPTION 30
#define VERSION_RELEASE_CODE_OPTION 40
#define VERSION_MAJOR_OPTION 41
#define VERSION_MINOR_OPTION 42

int
saftest_driver_client_main(int argc, char **argv,
                           void *first_request, int first_request_length)
{
    SaAisErrorT         status = 255;
    int                 daemon_flag = 0;
    int                 no_daemonize_flag = 0;
    int                 socket_file_flag = 0;
    int                 run_dir_flag = 0;
    int                 log_file_flag = 0;
    int                 pid_file_flag = 0;
    int                 op_name_flag = 0;
    int     	        queue_name_flag = 0;
    int                 timeout_flag = 0;
    int                 queue_open_cb_flag = 0;
    int                 queue_group_track_cb_flag = 0;
    int                 message_delivered_cb_flag = 0;
    int                 message_received_cb_flag = 0;
    int                 msg_resource_id_flag = 0;
    int     	        verbose_flag = 0;
    int     	        dispatch_type_flag = 0;
    int     	        null_msg_handle_flag = 0;
    int     	        null_callbacks_flag = 0;
    int     	        null_version_flag = 0;
    int     	        null_msg_id_flag = 0;
    int     	        null_msg_status_flag = 0;
    int     	        null_selection_object_flag = 0;
    int                 invocation_flag = 0;
    int                 version_release_code_flag = 0;
    int                 version_major_flag = 0;
    int                 version_minor_flag = 0;
    char                run_path[BUF_SIZE];
    char                pid_file[BUF_SIZE];
    char                log_file[BUF_SIZE];
    char                socket_file[BUF_SIZE];
    char             	op_name[BUF_SIZE];
    char             	queue_name[SA_MAX_NAME_LENGTH+1];
    ais_test_msg_request_op_t op;
    int                 timeout = 0;
    int                 msg_resource_id = 0;
    SaVersionT           sa_version;
    SaDispatchFlagsT    dispatch_flags = 0;
    SaInvocationT       invocation = 0;
    int next_option = 0;
    ais_test_msg_request_t *request;
    int client_fd;

    const struct option long_options[] = {
        { "help",     0, NULL, HELP_OPTION},
        { "daemon",   0, NULL, DAEMON_OPTION},
        { "no-daemonize", 0, NULL, NO_DAEMONIZE_OPTION},
        { "socket-file", 1, NULL, SOCKET_FILE_OPTION},
        { "run-dir", 1, NULL, RUN_DIR_OPTION},
        { "log-file", 1, NULL, LOG_FILE_OPTION},
        { "pid-file", 1, NULL, PID_FILE_OPTION},
        { "op", 1, NULL, OP_NAME_OPTION},
        { "set-queue-open-cb", 0, NULL, SET_QUEUE_OPEN_CB_OPTION},
        { "set-queue-group-track-cb", 0, NULL, SET_QUEUE_GROUP_TRACK_CB_OPTION},
        { "set-message-delivered-cb", 0, NULL, SET_MESSAGE_DELIVERED_CB_OPTION},
        { "set-message-received-cb", 0, NULL, SET_MESSAGE_RECEIVED_CB_OPTION},
        { "resource-id", 1, NULL, MSG_RESOURCE_ID_OPTION},
        { "queue-name", 1, NULL, QUEUE_NAME_OPTION},
        { "invocation", 1, NULL, INVOCATION_OPTION},
        { "timeout", 1, NULL, TIMEOUT_OPTION},
        { "verbose", 0, NULL, VERBOSE_OPTION},
        { "expected-status", 1, NULL, EXPECTED_MSG_STATUS_OPTION},
        { "dispatch-flags", 1, NULL, DISPATCH_FLAGS_OPTION},
        { "null-msg-handle", 0, NULL, NULL_MSG_HANDLE_OPTION},
        { "null-callbacks", 0, NULL, NULL_CALLBACKS_OPTION},
        { "null-version", 0, NULL, NULL_VERSION_OPTION},
        { "null-msg-id", 0, NULL, NULL_MSG_ID_OPTION},
        { "null-msg-status", 0, NULL, NULL_MSG_STATUS_OPTION},
        { "null-selection-object", 0, NULL, NULL_SELECTION_OBJECT_OPTION},
        { "version-release-code", 1, NULL, VERSION_RELEASE_CODE_OPTION},
        { "version-major-code", 1, NULL, VERSION_MAJOR_OPTION},
        { "version-minor-code", 1, NULL, VERSION_MINOR_OPTION},
        { NULL,       0, NULL, 0   }   /* Required at end of array.  */
    };

    memset(queue_name, 0, sizeof(queue_name));
    sa_version.releaseCode = AIS_B_RELEASE_CODE;
    sa_version.majorVersion = AIS_B_VERSION_MAJOR;
    sa_version.minorVersion = AIS_B_VERSION_MINOR;

    do {
        opterr = 0;
        next_option = getopt_long (argc, argv, "", long_options, NULL);
        switch (next_option) {
            case HELP_OPTION:
                usage();
                break;
            case DAEMON_OPTION:
                if (daemon_flag) {
                    usage();
                }
                daemon_flag++;
                break;
            case NO_DAEMONIZE_OPTION:
                if (no_daemonize_flag) {
                    usage();
                }
                no_daemonize_flag++;
                break;
            case SOCKET_FILE_OPTION:
                if (socket_file_flag) {
                    usage();
                }
                socket_file_flag++;
                strcpy(socket_file, optarg);
                break;
            case RUN_DIR_OPTION:
                if (run_dir_flag) {
                    usage();
                }
                run_dir_flag++;
                strcpy(run_path, optarg);
                break;
            case LOG_FILE_OPTION:
                if (log_file_flag) {
                    usage();
                }
                log_file_flag++;
                strcpy(log_file, optarg);
                break;
            case PID_FILE_OPTION:
                if (pid_file_flag) {
                    usage();
                }
                pid_file_flag++;
                strcpy(pid_file, optarg);
                break;
            case OP_NAME_OPTION:
                if (op_name_flag) {
                    usage();
                }
                op_name_flag++;
                strcpy(op_name, optarg);
                break;
            case QUEUE_NAME_OPTION:
                if (queue_name_flag) {
                    usage();
                }
                queue_name_flag++;
                strcpy(queue_name, optarg);
                break;
            case TIMEOUT_OPTION:
                if (timeout_flag) {
                    usage();
                }
                timeout_flag++;
                timeout = atoi(optarg);
                break;
            case SET_QUEUE_OPEN_CB_OPTION:
                if (queue_open_cb_flag) {
                    usage();
                }
                queue_open_cb_flag++;
                break;
            case SET_QUEUE_GROUP_TRACK_CB_OPTION:
                if (queue_group_track_cb_flag) {
                    usage();
                }
                queue_group_track_cb_flag++;
                break;
            case SET_MESSAGE_DELIVERED_CB_OPTION:
                if (message_delivered_cb_flag) {
                    usage();
                }
                message_delivered_cb_flag++;
                break;
            case SET_MESSAGE_RECEIVED_CB_OPTION:
                if (message_received_cb_flag) {
                    usage();
                }
                message_received_cb_flag++;
                break;
            case MSG_RESOURCE_ID_OPTION:
                if (msg_resource_id_flag) {
                    usage();
                }
                msg_resource_id_flag++;
                msg_resource_id = atoi(optarg);
                break;
            case VERBOSE_OPTION:
                if (verbose_flag) {
                    usage();
                }
                verbose_flag++;
                break;
            case DISPATCH_FLAGS_OPTION:
                if (dispatch_type_flag) {
                    usage();
                }
                dispatch_type_flag++;
                if (0 == strcmp(optarg, "SA_DISPATCH_ONE")) {
                    dispatch_flags = SA_DISPATCH_ONE;
                } else if (0 == strcmp(optarg, "SA_DISPATCH_ALL")) {
                    dispatch_flags = SA_DISPATCH_ALL;
                } else if (0 == strcmp(optarg, "SA_DISPATCH_BLOCKING")) {
                    dispatch_flags = SA_DISPATCH_BLOCKING;
                } else if (0 == strcmp(optarg, "SA_DISPATCH_INVALID")) {
                    dispatch_flags = -1;
                } else {
                    usage();
                }
                break;
            case NULL_MSG_HANDLE_OPTION:
                if (null_msg_handle_flag) {
                    usage();
                }
                null_msg_handle_flag++;
                break;
            case NULL_CALLBACKS_OPTION:
                if (null_callbacks_flag) {
                    usage();
                }
                null_callbacks_flag++;
                break;
            case NULL_VERSION_OPTION:
                if (null_version_flag) {
                    usage();
                }
                null_version_flag++;
                break;
            case NULL_MSG_ID_OPTION:
                if (null_msg_id_flag) {
                    usage();
                }
                null_msg_id_flag++;
                break;
            case NULL_MSG_STATUS_OPTION:
                if (null_msg_status_flag) {
                    usage();
                }
                null_msg_status_flag++;
                break;
            case NULL_SELECTION_OBJECT_OPTION:
                if (null_selection_object_flag) {
                    usage();
                }
                null_selection_object_flag++;
                break;
            case VERSION_RELEASE_CODE_OPTION:
                if (version_release_code_flag) {
                    usage();
                }
                version_release_code_flag++;
                sa_version.releaseCode = atoi(optarg);
                break;
            case VERSION_MAJOR_OPTION:
                if (version_major_flag) {
                    usage();
                }
                version_major_flag++;
                sa_version.majorVersion = atoi(optarg);
                break;
            case VERSION_MINOR_OPTION:
                if (version_minor_flag) {
                    usage();
                }
                version_minor_flag++;
                sa_version.minorVersion = atoi(optarg);
                break;
            case INVOCATION_OPTION:
                if (invocation_flag) {
                    usage();
                }
                invocation_flag++;
                invocation = atoi(optarg);
                break;
            case -1:
                /* No more options */
                break;
            default:
                usage();
        } /* switch (c) */
    } while (-1 != next_option);

    /*
     * A run path must always be specified
     */
    if (!run_dir_flag) {
        usage();
    }

    /* We must be a client that wants to talk to a daemon */
    if (!op_name_flag || !socket_file_flag) {
        usage();
    }

    ais_test_uds_connect(&client_fd, socket_file);
    ais_test_send_request(client_fd, first_request, first_request_length, 0);

    op = ais_test_map_string_to_op(op_name);

    request = (ais_test_msg_request_t *)
              malloc(sizeof(ais_test_msg_request_t));
    memset(request, 0, sizeof(ais_test_msg_request_t));
    request->op = op;
    request->requestor_pid = getpid();
    switch(op) {
        case AIS_TEST_MSG_REQUEST_CREATE_TEST_RESOURCE:
            status = 
                ais_test_client_handle_create_test_res_request(
                    client_fd, request);
            break;
        case AIS_TEST_MSG_REQUEST_MSG_INITALIZE:
            if (!msg_resource_id_flag || !dispatch_type_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_init_request(client_fd, 
                                                    request,
                                                    msg_resource_id,
                                                    &sa_version,
                                                    queue_open_cb_flag,
                                                    queue_group_track_cb_flag,
                                                    message_delivered_cb_flag,
                                                    message_received_cb_flag,
                                                    dispatch_flags,
                                                    null_msg_handle_flag,
                                                    null_callbacks_flag,
                                                    null_version_flag);
            break;
        case AIS_TEST_MSG_REQUEST_MSG_SELECTION_OBJECT_GET:
            if (!msg_resource_id_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_selection_object_request(
                    client_fd, request, msg_resource_id,
                    null_selection_object_flag);
            break;
        case AIS_TEST_MSG_REQUEST_DISPATCH:
            /*
             * Can specify either a specific handle or a msg_resource_id
             */
            if (!msg_resource_id_flag || !dispatch_type_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_dispatch_request(
                    client_fd, request, msg_resource_id, dispatch_flags);
            break;
        case AIS_TEST_MSG_REQUEST_MSG_FINALIZE:
            if (!msg_resource_id_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_resource_finalize_request(
                    client_fd, request, msg_resource_id);
            break;
        case AIS_TEST_MSG_REQUEST_MSG_QUEUE_OPEN:
            if (!msg_resource_id_flag || !queue_name_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_queue_open_request(
                    client_fd, request, msg_resource_id, queue_name);
            break;
        default:
            ais_test_abort("Client received request with unknown op %s\n",
                           op_name);
    }
    free(request);
    if (verbose_flag) {
        ais_test_log("Exit status for %s request is %s\n",
                    op_name, get_error_string(status));
    }

    exit(status);
}
