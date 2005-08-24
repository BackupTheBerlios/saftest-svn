/**********************************************************************
 *
 *	I N C L U D E S
 *
 **********************************************************************/
#include "saftest_driver_lib_utils.h"
#include "saftest_driver.h"
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

typedef struct msg_resource {
    int msg_resource_id;
    pthread_t thread_id;

    SaVersionT version;
    SaNameT entity_name;
    SaNameT queue_group_name;
    SaTimeT timeout;
    SaDispatchFlagsT dispatch_flags;
    SaMsgHandleT msg_handle;
    SaMsgQueueHandleT queue_handle;
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

    printf("Client Usage: msg_driver --o <op name>\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         [[--key <key> --value <value>] ... ]\n");
    printf("\n");
	exit(255);
}

const char *get_library_id()
{
    return "MSG";
}

void saftest_daemon_init(FILE *log_fp)
{
    assert(NULL != log_fp);
    saftest_log_set_fp(log_fp);
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
    saftest_log("Added a msg resource with id %d\n", res->msg_resource_id);
    return(res);
}

void
delete_msg_resource(msg_resource_t *res)
{
    saftest_log("Deleting msg resource with id %d\n", res->msg_resource_id);
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
saftest_daemon_queue_open_callback(SaInvocationT invocation,
                           SaMsgQueueHandleT queueHandle,
                           SaAisErrorT error)
{
    saftest_abort("Define me\n");
}

void
saftest_daemon_queue_group_track_callback(
    const SaNameT *queueGroupName,
    const SaMsgQueueGroupNotificationBufferT *notificationBuffer,
    SaUint32T numberOfMembers,
    SaAisErrorT error)
{
    saftest_abort("Define me\n");
}

void
saftest_daemon_message_delivered_callback(SaInvocationT invocation,
                                           SaAisErrorT error)
{
    saftest_abort("Define me\n");
}

void
saftest_daemon_message_received_callback(SaInvocationT invocation)
{
    saftest_abort("Define me\n");
}

static void *
saftest_daemon_dispatch_thread(void *arg)
{
    msg_resource_t *msg_res = (msg_resource_t *)arg;
    SaAisErrorT err;
    
    err = saMsgDispatch(msg_res->msg_handle, SA_DISPATCH_BLOCKING);
    if (err != SA_AIS_OK) {
        saftest_abort("Error %s calling "
                       "saMsgDispatch(SA_DISPATCH_BLOCKING)\n",
                       get_error_string(err));
    }
    return(NULL);
}

static SaDispatchFlagsT
saftest_daemon_get_dispatch_flags(const char *dispatch_flags_str)
{
    SaDispatchFlagsT flags;

    if (0 == strcmp(dispatch_flags_str, "SA_DISPATCH_ONE")) {
        flags = SA_DISPATCH_ONE;
    } else if (0 == strcmp(dispatch_flags_str, "SA_DISPATCH_ALL")) {
        flags = SA_DISPATCH_ALL;
    } else if (0 == strcmp(dispatch_flags_str, "SA_DISPATCH_BLOCKING")) {
        flags = SA_DISPATCH_BLOCKING;
    } else if (0 == strcmp(dispatch_flags_str, "SA_DISPATCH_INVALID")) {
        flags = -1;
    } else {
        saftest_abort("Unknown dispatch flags string %s\n", 
                      dispatch_flags_str);
    }
    return(flags);
}

void 
saftest_daemon_handle_create_test_res_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    msg_resource_t *msg_res;
    
    saftest_log("Received a create test resource request\n");

    msg_res = add_msg_resource();

    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        SA_AIS_OK);
    saftest_msg_set_ubit32_value((*reply), "MSG_RESOURCE_ID",
                                 msg_res->msg_resource_id);
}

void 
saftest_daemon_handle_init_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    msg_resource_t *msg_res;
    int err;
    SaMsgHandleT *handle = NULL;
    SaMsgCallbacksT *callbacks = NULL;
    SaVersionT *version = NULL;
    SaAisErrorT status;
    char *releaseCodeStr;
    char releaseCode;

    releaseCodeStr = saftest_msg_get_str_value(request,
                                               "VERSION_RELEASE_CODE");    
    releaseCode = releaseCodeStr[0];
    saftest_log("Received an init request from for id %d "
                 "release code=%c, majorVersion=%d, minorVersion=%d\n",
                 saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"),
                 releaseCode,
                 saftest_msg_get_ubit32_value(request, "VERSION_MAJOR"),
                 saftest_msg_get_ubit32_value(request, "VERSION_MINOR"));

    msg_res = lookup_msg_resource(
                  saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    if (NULL == msg_res) {
        saftest_abort("Unknown resource id %d\n",
                      saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    }

    if (0 == strcmp("FALSE", 
                    saftest_msg_get_str_value(request, "NULL_MSG_HANDLE"))) {
        handle = &msg_res->msg_handle;
    }
    if (0 == strcmp("FALSE", 
                    saftest_msg_get_str_value(request, "NULL_CALLBACKS"))) {
        callbacks = &msg_res->msg_callbacks;
        if (0 == strcmp("TRUE", 
                        saftest_msg_get_str_value(request, "QUEUE_OPEN_CB"))) {
            msg_res->msg_callbacks.saMsgQueueOpenCallback = 
                saftest_daemon_queue_open_callback;
        }
        if (0 == strcmp("TRUE", 
                        saftest_msg_get_str_value(request, 
                                                  "QUEUE_GROUP_TRACK_CB"))) {
            msg_res->msg_callbacks.saMsgQueueGroupTrackCallback = 
                saftest_daemon_queue_group_track_callback;
        }
        if (0 == strcmp("TRUE",
                        saftest_msg_get_str_value(request, 
                                                  "MESSAGE_DELIVERED_CB"))) {
            msg_res->msg_callbacks.saMsgMessageDeliveredCallback =
                saftest_daemon_message_delivered_callback;
        }
        if (0 == strcmp("TRUE",
                        saftest_msg_get_str_value(request, 
                                                  "MESSAGE_RECEIEVED_CB"))) {
            msg_res->msg_callbacks.saMsgMessageReceivedCallback =
                saftest_daemon_message_received_callback;
        }
    }
    if (0 == strcmp("FALSE", 
                    saftest_msg_get_str_value(request, "NULL_VERSION"))) {
        version = &msg_res->version;
    }

    msg_res->msg_callbacks.saMsgQueueOpenCallback = NULL;
    msg_res->msg_callbacks.saMsgQueueGroupTrackCallback = NULL;
    msg_res->msg_callbacks.saMsgMessageDeliveredCallback = NULL;
    msg_res->msg_callbacks.saMsgMessageReceivedCallback = NULL;

    msg_res->version.releaseCode = releaseCode;
    msg_res->version.majorVersion =
        (SaUint8T) saftest_msg_get_ubit32_value(request, 
                                                "VERSION_MAJOR");
    msg_res->version.minorVersion =
        (SaUint8T) saftest_msg_get_ubit32_value(request, 
                                                "VERSION_MINOR");
    msg_res->dispatch_flags = 
        saftest_daemon_get_dispatch_flags(
                   saftest_msg_get_str_value(request, "DISPATCH_FLAGS"));

    status = saMsgInitialize(handle, callbacks, version);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
    if (SA_AIS_OK == status) {
        if (msg_res->dispatch_flags == SA_DISPATCH_BLOCKING) {
            saftest_log("Starting new dispatch thread\n");
            err = pthread_create(&msg_res->thread_id, NULL, 
                                 saftest_daemon_dispatch_thread,
                                 (void*)msg_res);
            if (err) {
                saftest_abort("Error creating new thread: (%d) %s\n", 
                               errno, strerror(errno));
            }
        }
    }
}

void 
saftest_daemon_handle_selection_object_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    msg_resource_t *msg_res = NULL;
    SaSelectionObjectT *selection_object = NULL;
    SaAisErrorT status;

    saftest_log("Received a select object get request for id %d\n",
                saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    msg_res = lookup_msg_resource(
                  saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    if (NULL == msg_res) {
        saftest_abort("Unknown resource id %d\n",
                      saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    }
    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, 
                                              "NULL_SELECTION_OBJECT"))) {
        selection_object = &msg_res->selection_object;
    }
    status = saMsgSelectionObjectGet(msg_res->msg_handle, selection_object);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_dispatch_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    msg_resource_t *msg_res = NULL;
    SaAisErrorT status;
    SaDispatchFlagsT dispatch_flags;

    saftest_log("Received a dispatch request for id %d\n",
                saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    msg_res = lookup_msg_resource(
                  saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    if (NULL == msg_res) {
        saftest_abort("Unknown resource id %d\n",
                      saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    }
    dispatch_flags = 
        saftest_daemon_get_dispatch_flags(
                   saftest_msg_get_str_value(request, "DISPATCH_FLAGS"));
    status = saMsgDispatch(msg_res->msg_handle, dispatch_flags);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_resource_finalize_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    msg_resource_t *msg_res = NULL;
    SaAisErrorT status;

    saftest_log("Received a finalize request for id %d\n",
                saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    msg_res = lookup_msg_resource(
                  saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    if (NULL == msg_res) {
        saftest_abort("Unknown resource id %d\n",
                      saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    }
    status = saMsgFinalize(msg_res->msg_handle);
    msg_res->selection_object = 0;
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_queue_open_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    msg_resource_t *msg_res = NULL;
    SaAisErrorT status;
    SaNameT entity_name;
    SaMsgQueueCreationAttributesT creation_attrs;
    SaMsgQueueOpenFlagsT open_flags;
    char             	size_array_key_name[BUF_SIZE];
    int                 size_array_ndx = 0;

    saftest_log("Received a queue open request for id %d queue name %s\n",
                saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"),
                saftest_msg_get_str_value(request, "QUEUE_NAME"));
    msg_res = lookup_msg_resource(
                  saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    if (NULL == msg_res) {
        saftest_abort("Unknown resource id %d\n",
                      saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    }
    entity_name.length = 
        strlen(saftest_msg_get_str_value(request, "QUEUE_NAME"))+1;
    strncpy(entity_name.value, 
            saftest_msg_get_str_value(request, "QUEUE_NAME"),
            entity_name.length);

    memset(&creation_attrs, 0, sizeof(creation_attrs));
    if (0 == strcmp("TRUE", saftest_msg_get_str_value(request, "PERSISTENT"))) {
        creation_attrs.creationFlags |= SA_MSG_QUEUE_PERSISTENT;
    }
    for (size_array_ndx = 0; size_array_ndx <= SA_MSG_MESSAGE_LOWEST_PRIORITY;  
         size_array_ndx++) {
        sprintf(size_array_key_name, "SIZE_ARRAY_%d", size_array_ndx);
        creation_attrs.size[size_array_ndx] = 
            saftest_msg_get_ubit64_value(request, size_array_key_name);
    }
    creation_attrs.retentionTime = 
        saftest_msg_get_ubit64_value(request, "RETENTION_TIME");
    if (0 == strcmp("TRUE", saftest_msg_get_str_value(request, "CREATE"))) {
        open_flags |= SA_MSG_QUEUE_CREATE;
    }
    if (0 == strcmp("TRUE",  
                    saftest_msg_get_str_value(request, "RECEIVE_CALLBACK"))) {
        open_flags |= SA_MSG_QUEUE_RECEIVE_CALLBACK;
    }
    if (0 == strcmp("TRUE", saftest_msg_get_str_value(request, "EMPTY"))) {
        open_flags |= SA_MSG_QUEUE_EMPTY;
    }
    
    status = saMsgQueueOpen(msg_res->msg_handle, &entity_name,
                            &creation_attrs, open_flags,
                            0, &(msg_res->queue_handle)); 
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_message_send_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    msg_resource_t *msg_res = NULL;
    SaAisErrorT status;
    SaNameT entity_name;
    SaNameT sender_name;
    SaMsgMessageT message;
    SaTimeT timeout = 0;

    saftest_log("Received a message send request for id %d entity name %s\n",
                saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"),
                saftest_msg_get_str_value(request, "ENTITY_NAME"));
    msg_res = lookup_msg_resource(
                  saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    if (NULL == msg_res) {
        saftest_abort("Unknown resource id %d\n",
                      saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    }
    entity_name.length = 
        strlen(saftest_msg_get_str_value(request, "ENTITY_NAME"))+1;
    strncpy(entity_name.value, 
            saftest_msg_get_str_value(request, "ENTITY_NAME"),
            entity_name.length);
    sender_name.length = 
        strlen(saftest_msg_get_str_value(request, "SENDER_NAME"))+1;
    strncpy(sender_name.value, 
            saftest_msg_get_str_value(request, "SENDER_NAME"),
            sender_name.length);

    memset(&message, 0, sizeof(message));
    message.type = saftest_msg_get_ubit32_value(request, "MSG_TYPE");
    message.version = saftest_msg_get_ubit32_value(request, "MSG_VERSION");
    message.size = strlen(saftest_msg_get_str_value(request, "MSG_STRING"));
    message.data = saftest_msg_get_str_value(request, "MSG_STRING");
    message.senderName = &sender_name;
    message.priority = saftest_msg_get_ubit32_value(request, "MSG_PRIORITY");
    status = saMsgMessageSend(msg_res->msg_handle, &entity_name, 
                              &message, timeout);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_message_get_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    msg_resource_t *msg_res = NULL;
    SaNameT entity_name;
    SaNameT sender_name;
    SaMsgMessageT message;
    SaMsgSenderIdT sender_id = 0;
    SaTimeT send_time = 0;
    SaTimeT timeout = 0;
    SaAisErrorT status;
    char msg_string[SAFTEST_STRING_LENGTH+1];
    char sender_name_str[SAFTEST_STRING_LENGTH+1];

    saftest_log("Received a message get request for id %d entity name %s\n",
                saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"),
                saftest_msg_get_str_value(request, "ENTITY_NAME"));
    msg_res = lookup_msg_resource(
                  saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    if (NULL == msg_res) {
        saftest_abort("Unknown resource id %d\n",
                      saftest_msg_get_ubit32_value(request, "MSG_RESOURCE_ID"));
    }

    memset(&message, 0, sizeof(message));
    memset(msg_string, 0, sizeof(msg_string));
    memset(sender_name_str, 0, sizeof(sender_name_str));
    entity_name.length = 
        strlen(saftest_msg_get_str_value(request, "ENTITY_NAME"))+1;
    strncpy(entity_name.value, 
            saftest_msg_get_str_value(request, "ENTITY_NAME"),
            entity_name.length);
    message.senderName = &sender_name;

    status = saMsgMessageGet(msg_res->msg_handle, 
                             &message, &send_time, &sender_id, timeout);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
    if (SA_AIS_OK == status) {
        saftest_msg_set_ubit32_value(*reply, "MSG_TYPE", message.type);
        saftest_msg_set_ubit32_value(*reply, "MSG_VERSION", message.version);
        saftest_msg_set_ubit32_value(*reply, "MSG_PRIORITY", message.priority);
        saftest_msg_set_ubit32_value(*reply, "MSG_SIZE", message.size);
        memcpy(msg_string, message.data, message.size);
        saftest_msg_set_str_value(*reply, "MSG_STRING", msg_string);
        strncpy(sender_name_str, message.senderName->value,
                message.senderName->length);
        saftest_msg_set_str_value(*reply, "SENDER_NAME", sender_name_str);
    }
}

void
saftest_daemon_handle_incoming_msg_message(gpointer data, gpointer user_data)
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

    saftest_log("Incoming request on msg selection fd %d\n",
                 msg_res->selection_object);
    err = saMsgDispatch(msg_res->msg_handle, msg_res->dispatch_flags);
    if (SA_AIS_OK != err) {
        saftest_log("Error %s performing saMsgDispatch\n",
                     get_error_string(err));
    }
}

void saftest_daemon_add_msg_resource_to_fdset(gpointer data, 
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

void saftest_daemon_add_fds(
    int *max_fd,
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    fd_set_key_t set_key;

    set_key.set = read_fd_set;
    set_key.largest_fd = *max_fd;

    g_list_foreach(msg_list, saftest_daemon_add_msg_resource_to_fdset,
                   &set_key);
    *max_fd = set_key.largest_fd;
}

void
saftest_daemon_check_fds(
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    g_list_foreach(msg_list,
                   saftest_daemon_handle_incoming_msg_message,
                   read_fd_set);
}

SaAisErrorT
saftest_client_handle_create_test_res_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply = NULL;
    SaAisErrorT status;
 
    saftest_send_request(fd, get_library_id(), request, &reply);
    if (NULL == reply) {
        saftest_abort("Received no reply from the daemon\n");
    }

    status = saftest_reply_msg_get_status(reply);
    if (SA_AIS_OK == status) {
        saftest_log("Resource ID=%d\n", 
                    saftest_msg_get_ubit32_value(reply, "MSG_RESOURCE_ID"));
    }
    free(reply);

    return(status);
}

SaAisErrorT
saftest_client_handle_message_get_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply;
    SaAisErrorT status;
 
    saftest_send_request(fd, get_library_id(), request, &reply);
    if (NULL == reply) {
        saftest_abort("Received no reply from the daemon\n");
    }
    status = saftest_reply_msg_get_status(reply);
    free(reply);
    if (SA_AIS_OK == status) {
        saftest_log("Message Type=%d\n", 
                    saftest_msg_get_ubit32_value(reply, "MSG_TYPE"));
        saftest_log("Message Version=%d\n",
                    saftest_msg_get_ubit32_value(reply, "MSG_VERSION"));
        saftest_log("Message Size=%d\n",
                    saftest_msg_get_ubit32_value(reply, "MSG_SIZE"));
        saftest_log("Message Priority=%d\n",
                    saftest_msg_get_ubit32_value(reply, "MSG_PRIORITY"));
        saftest_log("Sender Name=%s\n",
                    saftest_msg_get_str_value(reply, "SENDER_NAME"));
        saftest_log("Message String=%s\n",
                    saftest_msg_get_str_value(reply, "MSG_STRING"));
    }
    return(status);
}

SAFTEST_MAP_TABLE_BEGIN()
SAFTEST_MAP_TABLE_ENTRY(
    "CREATE_TEST_RESOURCE_REQ", "CREATE_TEST_RESOURCE_REPLY",
    saftest_client_handle_create_test_res_request,
    saftest_daemon_handle_create_test_res_request)

SAFTEST_MAP_TABLE_ENTRY(
    "MSG_INITIALIZE_REQ", "MSG_INITIALIZE_REPLY",
    saftest_client_generic_handle_request,
    saftest_daemon_handle_init_request)

SAFTEST_MAP_TABLE_ENTRY(
    "SELECTION_OBJECT_GET_REQ", "SELECTION_OBJECT_GET_REQ",
     saftest_client_generic_handle_request,
     saftest_daemon_handle_selection_object_request)

SAFTEST_MAP_TABLE_ENTRY(
    "DISPATCH_REQ", "DISPATCH_REPLY",
     saftest_client_generic_handle_request,
     saftest_daemon_handle_dispatch_request)

SAFTEST_MAP_TABLE_ENTRY(
    "FINALIZE_REQ", "FINALIZE_REPLY",
     saftest_client_generic_handle_request,
     saftest_daemon_handle_resource_finalize_request)

SAFTEST_MAP_TABLE_ENTRY(
    "QUEUE_OPEN_REQ", "QUEUE_OPEN_REPLY",
     saftest_client_generic_handle_request,
     saftest_daemon_handle_queue_open_request)

SAFTEST_MAP_TABLE_ENTRY(
    "MESSAGE_SEND_REQ", "MESSAGE_SEND_REPLY",
     saftest_client_generic_handle_request,
     saftest_daemon_handle_message_send_request)

SAFTEST_MAP_TABLE_ENTRY(
    "MESSAGE_GET_REQ", "MESSAGE_GET_REPLY",
     saftest_client_handle_message_get_request,
     saftest_daemon_handle_message_get_request)
SAFTEST_MAP_TABLE_END()
