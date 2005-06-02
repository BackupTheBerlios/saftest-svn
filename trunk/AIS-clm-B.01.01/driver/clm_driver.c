/**********************************************************************
 *
 *	I N C L U D E S
 *
 **********************************************************************/
#include "saftest_driver.h"
#include "saClm.h"

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

enum ais_test_clm_request_op {
    AIS_TEST_CLM_REQUEST_INVALID=0,
    AIS_TEST_CLM_REQUEST_CREATE_TEST_RESOURCE,
    AIS_TEST_CLM_REQUEST_INITALIZE,
    AIS_TEST_CLM_REQUEST_FINALIZE,
    AIS_TEST_CLM_REQUEST_SELECTION_OBJECT_GET,
    AIS_TEST_CLM_REQUEST_DISPATCH,
    AIS_TEST_CLM_REQUEST_CLUSTER_NODE_GET,
    AIS_TEST_CLM_REQUEST_CLUSTER_NODE_GET_ASYNC,
    AIS_TEST_CLM_REQUEST_CLUSTER_NODE_GET_CALLBACK_COUNT,
    AIS_TEST_CLM_REQUEST_CLUSTER_NODE_GET_ASYNC_INVOCATION,
    AIS_TEST_CLM_REQUEST_CLUSTER_TRACK,
    AIS_TEST_CLM_REQUEST_CLUSTER_TRACK_STOP,
    AIS_TEST_CLM_REQUEST_CLUSTER_TRACK_CALLBACK_COUNT,
    AIS_TEST_CLM_REQUEST_DISPLAY_LAST_NOTIFICATION_BUFFER,
};
typedef enum ais_test_clm_request_op ais_test_clm_request_op_t;

typedef struct ais_test_clm_request {
    ais_test_clm_request_op_t op;
    pid_t requestor_pid;
    SaVersionT sa_version;
    int cluster_node_get_cb_flag;
    int cluster_track_cb_flag;
    SaDispatchFlagsT dispatch_flags;
    SaUint8T track_flags;
    SaUint32T number_of_items;
    int clm_resource_id;
    SaInvocationT invocation;
    SaClmNodeIdT node_id;
    SaTimeT timeout;
    int null_clm_handle_flag;
    int null_callbacks_flag;
    int null_version_flag;
    int null_selection_object_flag;
    int null_cluster_node_flag;
    int null_notification_buffer_flag;
    int null_cluster_notification_flag;
    char xml_file[BUF_SIZE];
} ais_test_clm_request_t;

typedef struct ais_test_clm_reply {
    SaAisErrorT status;
    int clm_resource_id;
    int cluster_node_get_callback_count;
    int cluster_track_callback_count;
    SaInvocationT cluster_node_get_async_invocation;
} ais_test_clm_reply_t;

typedef struct clm_resource {
    int clm_resource_id;
    pthread_t thread_id;

    SaVersionT version;
    SaTimeT timeout;
    SaDispatchFlagsT dispatch_flags;
    SaClmHandleT clm_handle;
    SaSelectionObjectT selection_object;
    SaUint8T track_flags;
    SaClmCallbacksT clm_callbacks;
    SaClmClusterNotificationBufferT notification_buffer;

    SaInvocationT cluster_node_get_async_invocation;
    int cluster_node_get_callback_count;
    int cluster_track_callback_count;
} clm_resource_t;

GList *clm_list = NULL;

void
usage()
{
    printf("Server Usage: clm_driver --daemon\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --log-file <log file>\n");
    printf("                         --pid-file <pid file>\n");
    printf("                        [-no-daemonize]\n");

    printf("\n");

    printf("Client Usage: clm_driver --o CREATE_TEST_RES\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("\n");
    printf("Client Usage: clm_driver --o INIT\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --dispatch-flags \n");
    printf("                           <SA_CLM_DISPATCH_BLOCKING |\n");
    printf("                            SA_CLM_DISPATCH_ONE |\n");
    printf("                            SA_CLM_DISPATCH_ALL>  \n");
    printf("                        [--set-cluster-node-get-cb]\n");
    printf("                        [--set-cluster-track-cb]\n");
    printf("                        [--version-release-code <code #>]\n");
    printf("                        [--version-major <code #>]\n");
    printf("                        [--version-minor <code #>]\n");
    printf("                        [--null-clm-handle]\n");
    printf("                        [--null-callbacks]\n");
    printf("                        [--null-version]\n");
    printf("\n");
    printf("Client Usage: clm_driver --o SELECT_OBJ_GET\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                        [--null-selection-object]\n");
    printf("\n");
    printf("Client Usage: clm_driver --o FINALIZE\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("\n");
    printf("Client Usage: clm_driver --o CLUSTER_NODE_GET\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                         --node-id <node id>\n");
    printf("                         --timeout <timeout>\n");
    printf("                        [--null-cluster-node]\n");
    printf("\n");
    printf("Client Usage: clm_driver --o CLUSTER_NODE_GET_ASYNC\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                         --invocation <invocation>\n");
    printf("                         --node-id <node id>\n");
    printf("\n");
    printf("Client Usage: clm_driver --o CLUSTER_NODE_GET_CALLBACK_COUNT\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("\n");
    printf("Client Usage: clm_driver --o CLUSTER_NODE_GET_ASYNC_INVOCATION\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("\n");
    printf("Client Usage: clm_driver --o CLUSTER_TRACK\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                        [--invalid-track-flags]\n");
    printf("                        [--track-current]\n");
    printf("                        [--track-changes]\n");
    printf("                        [--track-changes-only]\n");
    printf("                        [--null-notification-buffer]\n");
    printf("                        [--null-cluster-notification]\n");
    printf("                        [--number-of-items]\n");
    printf("\n");
    printf("Client Usage: clm_driver --o CLUSTER_TRACK_CALLBACK_COUNT\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("\n");
    printf("Client Usage: clm_driver --o DISPATCH\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                         --dispatch-flags \n");
    printf("                           <SA_CLM_DISPATCH_BLOCKING |\n");
    printf("                            SA_CLM_DISPATCH_ONE |\n");
    printf("                            SA_CLM_DISPATCH_ALL>  \n");
    printf("\n");
    printf("Client Usage: clm_driver --o DISPLAY_LAST_NOTIFICATION_BUFFER\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                         --xml-file <xml path>\n");
    printf("\n");
	exit(255);
}

static ais_test_clm_request_op_t
ais_test_map_string_to_op(const char *str)
{
    if (0 == strcmp(str, "CREATE_TEST_RES")) {
        return AIS_TEST_CLM_REQUEST_CREATE_TEST_RESOURCE;
    } else if (0 == strcmp(str, "INIT")) {
        return AIS_TEST_CLM_REQUEST_INITALIZE;
    } else if (0 == strcmp(str, "SELECT_OBJ_GET")) {
        return AIS_TEST_CLM_REQUEST_SELECTION_OBJECT_GET;
    } else if (0 == strcmp(str, "FINALIZE")) {
        return AIS_TEST_CLM_REQUEST_FINALIZE;
    } else if (0 == strcmp(str, "DISPATCH")) {
        return AIS_TEST_CLM_REQUEST_DISPATCH;
    } else if (0 == strcmp(str, "CLUSTER_NODE_GET")) {
        return AIS_TEST_CLM_REQUEST_CLUSTER_NODE_GET;
    } else if (0 == strcmp(str, "CLUSTER_NODE_GET_ASYNC")) {
        return AIS_TEST_CLM_REQUEST_CLUSTER_NODE_GET_ASYNC;
    } else if (0 == strcmp(str, "CLUSTER_NODE_GET_CALLBACK_COUNT")) {
        return AIS_TEST_CLM_REQUEST_CLUSTER_NODE_GET_CALLBACK_COUNT;
    } else if (0 == strcmp(str, "CLUSTER_NODE_GET_ASYNC_INVOCATION")) {
        return AIS_TEST_CLM_REQUEST_CLUSTER_NODE_GET_ASYNC_INVOCATION;
    } else if (0 == strcmp(str, "CLUSTER_TRACK")) {
        return AIS_TEST_CLM_REQUEST_CLUSTER_TRACK;
    } else if (0 == strcmp(str, "CLUSTER_TRACK_STOP")) {
        return AIS_TEST_CLM_REQUEST_CLUSTER_TRACK_STOP;
    } else if (0 == strcmp(str, "CLUSTER_TRACK_CALLBACK_COUNT")) {
        return AIS_TEST_CLM_REQUEST_CLUSTER_TRACK_CALLBACK_COUNT;
    } else if (0 == strcmp(str, "DISPLAY_LAST_NOTIFICATION_BUFFER")) {
        return AIS_TEST_CLM_REQUEST_DISPLAY_LAST_NOTIFICATION_BUFFER;
    }

    ais_test_abort("Unknown op string %s\n", str);
    return AIS_TEST_CLM_REQUEST_INVALID;
}

const char *get_library_id()
{
    return "CLM";
}

int get_library_message_size()
{
    return sizeof(ais_test_clm_request_t);
}

int
get_next_clm_resource_id()
{
    static int next_clm_resource_id = 1;
    int ret_id;

    ret_id = next_clm_resource_id;
    next_clm_resource_id += 1;
    return(ret_id);
}

clm_resource_t *
add_clm_resource()
{
    clm_resource_t *res;

    res = malloc(sizeof(clm_resource_t));
    assert(NULL != res);
    memset(res, 0, sizeof(clm_resource_t));

    clm_list = g_list_append(clm_list, res);
    res->clm_resource_id = get_next_clm_resource_id();
    ais_test_log("Added a clm resource with id %d\n", res->clm_resource_id);
    return(res);
}

void
delete_clm_resource(clm_resource_t *res)
{
    ais_test_log("Deleting clm resource with id %d\n", res->clm_resource_id);
    clm_list = g_list_remove(clm_list, res);
    free(res);
}

clm_resource_t *
lookup_clm_resource(int clm_resource_id)
{
    GList *element;
    clm_resource_t *res;

    for (element = g_list_first(clm_list);
         NULL != element;
         element = g_list_next(element)) {
        res = (clm_resource_t *)element->data;
        if (res->clm_resource_id == clm_resource_id) {
            return(res);
        }
    }
    return(NULL);
}

clm_resource_t *
lookup_clm_resource_by_invocation(SaInvocationT invocation)
{
    GList *element;
    clm_resource_t *res;

    for (element = g_list_first(clm_list);
         NULL != element;
         element = g_list_next(element)) {
        res = (clm_resource_t *)element->data;
        if (res->cluster_node_get_async_invocation == invocation) {
            return(res);
        }
    }
    return(NULL);
}

clm_resource_t *
lookup_clm_resource_with_track_callback()
{
    GList *element;
    clm_resource_t *res;

    for (element = g_list_first(clm_list);
         NULL != element;
         element = g_list_next(element)) {
        res = (clm_resource_t *)element->data;
        if (NULL != res->clm_callbacks.saClmClusterTrackCallback) {
            return(res);
        }
    }
    return(NULL);
}

void
ais_test_daemon_cluster_node_get_callback(SaInvocationT invocation,
                                          const SaClmClusterNodeT *clusterNode,
                                          SaAisErrorT error)
{
    clm_resource_t *clm_res = NULL;

    ais_test_log("Cluster Node Get Callback for invocation %lld\n", invocation);
    clm_res = lookup_clm_resource_by_invocation(invocation);
    if (NULL == clm_res) {
        ais_test_abort("Unknown invocation id %d\n",
                       invocation);
    }
    clm_res->cluster_node_get_callback_count += 1;
}

void
ais_test_daemon_cluster_track_callback(
    const SaClmClusterNotificationBufferT *notificationBuffer,
    SaUint32T numberOfMembers,
    SaAisErrorT error)
{
    clm_resource_t *clm_res = NULL;

    clm_res = lookup_clm_resource_with_track_callback();
    if (NULL == clm_res) {
        ais_test_abort("Unable to find a clm_resource with track callback\n");
    }
    ais_test_log("Received a cluster track callback\n");
    clm_res->cluster_track_callback_count += 1;
    /* This is a vicious, dirty hack that causes memory leaks and stuff */
    clm_res->notification_buffer = *notificationBuffer;
}

static void *
ais_test_daemon_dispatch_thread(void *arg)
{
    clm_resource_t *clm_res = (clm_resource_t *)arg;
    SaAisErrorT err;
    
    err = saClmDispatch(clm_res->clm_handle, SA_DISPATCH_BLOCKING);
    if (err != SA_AIS_OK) {
        ais_test_abort("Error %s calling "
                       "saClmDispatch(SA_DISPATCH_BLOCKING)\n",
                       get_error_string(err));
    }
    return(NULL);
}

void 
ais_test_daemon_handle_create_test_res_request(
    ais_test_clm_request_t *request,
    ais_test_clm_reply_t *reply)
{
    clm_resource_t *clm_res;
    
    ais_test_log("Received a create test resource request from pid %d.\n",
                 request->requestor_pid);

    clm_res = add_clm_resource();

    reply->clm_resource_id = clm_res->clm_resource_id;
    reply->status = SA_AIS_OK;
}

void 
ais_test_daemon_handle_init_request(ais_test_clm_request_t *request,
                                    ais_test_clm_reply_t *reply)
{
    clm_resource_t *clm_res;
    clm_resource_t *tmp_res;
    int err;
    SaClmHandleT *handle = NULL;
    SaClmCallbacksT *callbacks = NULL;
    SaVersionT *version = NULL;
    
    ais_test_log("Received an init request from pid %d for id %d "
                 "release code=%d, majorVersion=%d, minorVersion=%d\n",
                 request->requestor_pid,
                 request->clm_resource_id,
                 request->sa_version.releaseCode,
                 request->sa_version.majorVersion,
                 request->sa_version.minorVersion);

    clm_res = lookup_clm_resource(request->clm_resource_id);
    if (NULL == clm_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->clm_resource_id);
    }

    if (0 == request->null_clm_handle_flag) {
        handle = &clm_res->clm_handle;
    }
    if (0 == request->null_callbacks_flag) {
        callbacks = &clm_res->clm_callbacks;
    }
    if (0 == request->null_version_flag) {
        version = &clm_res->version;
    }

    if (request->cluster_node_get_cb_flag) {
        clm_res->clm_callbacks.saClmClusterNodeGetCallback =
            ais_test_daemon_cluster_node_get_callback;
    } else {
        clm_res->clm_callbacks.saClmClusterNodeGetCallback = NULL;
    }
    if (request->cluster_track_cb_flag) {
        tmp_res = lookup_clm_resource_with_track_callback();
        if (NULL != tmp_res) {
            ais_test_abort("Unable to create more than one resource with a "
                           "track callback\n");
        }
        clm_res->clm_callbacks.saClmClusterTrackCallback =
            ais_test_daemon_cluster_track_callback;
    } else {
        clm_res->clm_callbacks.saClmClusterTrackCallback = NULL;
    }

    clm_res->version = request->sa_version;
    clm_res->dispatch_flags = request->dispatch_flags;

    reply->status = saClmInitialize(handle, callbacks, version);
    ais_test_log("status is %d\n", reply->status);
    if (SA_AIS_OK == reply->status) {
        if (request->dispatch_flags == SA_DISPATCH_BLOCKING) {
            ais_test_log("Starting new dispatch thread\n");
            err = pthread_create(&clm_res->thread_id, NULL, 
                                 ais_test_daemon_dispatch_thread,
                                 (void*)clm_res);
            if (err) {
                ais_test_abort("Error creating new thread: (%d) %s\n", 
                               errno, strerror(errno));
            }
        }
    }
}

void 
ais_test_daemon_handle_selection_object_request(ais_test_clm_request_t *request,
                                                ais_test_clm_reply_t *reply)
{
    clm_resource_t *clm_res = NULL;
    SaSelectionObjectT *selection_object = NULL;

    ais_test_log("Received a select object get request for id %d\n",
                 request->clm_resource_id);
    clm_res = lookup_clm_resource(request->clm_resource_id);
    if (0 == request->null_selection_object_flag) {
        selection_object = &clm_res->selection_object;
    }
    if (NULL == clm_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->clm_resource_id);
    }
    reply->status = saClmSelectionObjectGet(clm_res->clm_handle, 
                                            selection_object);
}

void
ais_test_daemon_handle_cluster_node_get_request(
    ais_test_clm_request_t *request, 
    ais_test_clm_reply_t *reply)
{
    clm_resource_t *clm_res = NULL;
    SaClmClusterNodeT cluster_node;
    SaClmClusterNodeT *cluster_node_ptr = NULL;

    ais_test_log("Received a cluster node get request for id %d\n",
                 request->clm_resource_id);
    clm_res = lookup_clm_resource(request->clm_resource_id);
    if (NULL == clm_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->clm_resource_id);
    }
    if (0 == request->null_cluster_node_flag) {
        cluster_node_ptr = &cluster_node;
    }
    /*
     * !!! Need to address this timeout problem.  What is a timeout of 0
     * supposed to represent?
     */
    reply->status = saClmClusterNodeGet(clm_res->clm_handle, 
                                        request->node_id, request->timeout, 
                                        cluster_node_ptr);
}

void
ais_test_daemon_handle_cluster_node_get_async_request(
    ais_test_clm_request_t *request, 
    ais_test_clm_reply_t *reply)
{
    clm_resource_t *clm_res = NULL;

    ais_test_log("Received a cluster node get async request for id %d\n",
                 request->clm_resource_id);
    clm_res = lookup_clm_resource(request->clm_resource_id);
    if (NULL == clm_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->clm_resource_id);
    }
    clm_res->cluster_node_get_async_invocation = request->invocation;
    reply->status = saClmClusterNodeGetAsync(clm_res->clm_handle, 
                                             request->invocation,
                                             request->node_id);
}

void
ais_test_daemon_handle_cluster_node_get_cb_count_request(
    ais_test_clm_request_t *request, 
    ais_test_clm_reply_t *reply)
{
    clm_resource_t *clm_res = NULL;

    ais_test_log("Received a cluster node get cb count request for id %d\n",
                 request->clm_resource_id);
    clm_res = lookup_clm_resource(request->clm_resource_id);
    if (NULL == clm_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->clm_resource_id);
    }
    reply->cluster_node_get_callback_count =
        clm_res->cluster_node_get_callback_count;
    reply->status = SA_AIS_OK;
}

void
ais_test_daemon_handle_cluster_node_get_async_invocation_request(
    ais_test_clm_request_t *request, 
    ais_test_clm_reply_t *reply)
{
    clm_resource_t *clm_res = NULL;

    ais_test_log("Received a cluster node get async invocation request for id "
                 "%d\n",
                 request->clm_resource_id);
    clm_res = lookup_clm_resource(request->clm_resource_id);
    if (NULL == clm_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->clm_resource_id);
    }
    reply->cluster_node_get_async_invocation =
        clm_res->cluster_node_get_async_invocation;
    reply->status = SA_AIS_OK;
}

void
ais_test_daemon_handle_cluster_track_request(
    ais_test_clm_request_t *request, 
    ais_test_clm_reply_t *reply)
{
    clm_resource_t *clm_res = NULL;
    SaClmClusterNotificationBufferT *buffer = NULL;

    ais_test_log("Received a cluster track request for id %d\n",
                 request->clm_resource_id);
    clm_res = lookup_clm_resource(request->clm_resource_id);
    if (NULL == clm_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->clm_resource_id);
    }
    if (0 == request->null_notification_buffer_flag) {
        buffer = &clm_res->notification_buffer;
        if (0 == request->null_cluster_notification_flag) {
            buffer->numberOfItems = request->number_of_items;
            if (request->number_of_items == 0) {
                /* 
                 * This is a special unit test case, setting a pointer for
                 * notification_buffer->notification but setting numberOfItems
                 * to be 0.
                 */
                buffer->notification = (SaClmClusterNotificationT *)buffer;
            } else {
                buffer->notification = 
                    (SaClmClusterNotificationT *)
                    malloc(request->number_of_items *
                           sizeof(SaClmClusterNotificationT));
                if (NULL == buffer->notification) {
                    ais_test_abort("unable to allocate cluster notification array");
                }
            }
        }
    }
    clm_res->track_flags = request->track_flags;
    reply->status = saClmClusterTrack(clm_res->clm_handle, 
                                      clm_res->track_flags,
                                      buffer);
}

void
ais_test_daemon_handle_cluster_track_stop_request(
    ais_test_clm_request_t *request, 
    ais_test_clm_reply_t *reply)
{
    clm_resource_t *clm_res = NULL;

    ais_test_log("Received a cluster track stop request for id %d\n",
                 request->clm_resource_id);
    clm_res = lookup_clm_resource(request->clm_resource_id);
    if (NULL == clm_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->clm_resource_id);
    }
    reply->status = saClmClusterTrackStop(clm_res->clm_handle);
}

void
ais_test_daemon_handle_cluster_track_cb_count_request(
    ais_test_clm_request_t *request, 
    ais_test_clm_reply_t *reply)
{
    clm_resource_t *clm_res = NULL;

    ais_test_log("Received a cluster track count request for id %d\n",
                 request->clm_resource_id);
    clm_res = lookup_clm_resource(request->clm_resource_id);
    if (NULL == clm_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->clm_resource_id);
    }
    reply->cluster_track_callback_count =
        clm_res->cluster_track_callback_count;
    reply->status = SA_AIS_OK;
}

void 
ais_test_daemon_handle_finalize_request(ais_test_clm_request_t *request,
                                        ais_test_clm_reply_t *reply)
{
    clm_resource_t *clm_res = NULL;

    ais_test_log("Received a finalize request for id %d\n",
                 request->clm_resource_id);
    clm_res = lookup_clm_resource(request->clm_resource_id);
    if (NULL == clm_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->clm_resource_id);
    }
    reply->status = saClmFinalize(clm_res->clm_handle);
    clm_res->selection_object = 0;
}

void 
ais_test_daemon_handle_dispatch_request(ais_test_clm_request_t *request,
                                        ais_test_clm_reply_t *reply)
{
    clm_resource_t *clm_res = NULL;

    ais_test_log("Received a dispatch request for id %d\n",
                 request->clm_resource_id);
    clm_res = lookup_clm_resource(request->clm_resource_id);
    if (NULL == clm_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->clm_resource_id);
    }
    reply->status = saClmDispatch(clm_res->clm_handle, 
                                  request->dispatch_flags);
}

void 
ais_test_daemon_handle_display_last_notification_buffer_request(
    ais_test_clm_request_t *request,
    ais_test_clm_reply_t *reply)
{
    clm_resource_t *clm_res = NULL;
    char node_name[SA_MAX_NAME_LENGTH+1];
    FILE *fp = NULL;
    const char *family;
    SaUint32T ndx;
    struct in_addr in_addr;
    char addr_buf[INET6_ADDRSTRLEN];

    ais_test_log("Received a display last notification buffer request for "
                 "id %d to XML file %s\n",
                 request->clm_resource_id, request->xml_file);
    memset(node_name, 0, sizeof(node_name));
    if (0 == strlen(request->xml_file)) {
        ais_test_abort("XML file not specified\n");

    }
    clm_res = lookup_clm_resource(request->clm_resource_id);
    if (NULL == clm_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->clm_resource_id);
    }
    fp = fopen(request->xml_file, "w+");
    if (NULL == fp) {
        ais_test_abort("Unable to open %s for writing\n",
                       request->xml_file);
    }

    fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(fp, "<SAFCluster "
                " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                " xsi:noNamespaceSchemaLocation=\"SAFCluster.xsd\" "
                " schemaVersion=\"1\"> \n");

    fprintf(fp, "    <SAFNodeList>\n");
    
    for (ndx = 0; ndx < clm_res->notification_buffer.numberOfItems; ndx++) {
        memcpy(node_name, 
               clm_res->notification_buffer.notification[ndx].clusterNode.nodeName.value,
               clm_res->notification_buffer.notification[ndx].clusterNode.nodeName.length);
        fprintf(fp, "        <SAFNode>\n");
        fprintf(fp, "            <id>%ld</id>\n", 
                clm_res->notification_buffer.notification[ndx].clusterNode.nodeId);
        fprintf(fp, "            <AddressList>\n");
        fprintf(fp, "                <Address>\n");
        if (SA_CLM_AF_INET ==
            clm_res->notification_buffer.notification[ndx].clusterNode.nodeAddress.family) {
            family = "SA_CLM_AF_INET";
        } else if (SA_CLM_AF_INET ==
                   clm_res->notification_buffer.notification[ndx].clusterNode.nodeAddress.family) {
            family = "SA_CLM_AF_INET6";
        } else {
            ais_test_abort("Unknown address family\n");
        }
        fprintf(fp, "                    <family>%s</family>\n", family);
        fprintf(fp, "                    <length>%d</length>\n",
                clm_res->notification_buffer.notification[ndx].clusterNode.nodeAddress.length);

        memcpy(&in_addr.s_addr,
                clm_res->notification_buffer.notification[ndx].clusterNode.nodeAddress.value, 
                clm_res->notification_buffer.notification[ndx].clusterNode.nodeAddress.length);
        strcpy(addr_buf, inet_ntoa(in_addr));
        fprintf(fp, "                    <value>%s</value>\n",
                addr_buf);
        fprintf(fp, "                </Address>\n");
        fprintf(fp, "            </AddressList>\n");
        fprintf(fp, "            <name>%s</name>\n", 
                node_name);
        fprintf(fp, "            <member>%s</member>\n", 
                clm_res->notification_buffer.notification[ndx].clusterNode.member ?
                "TRUE" : "FALSE");
        fprintf(fp, "            <bootTimestamp>%lld</bootTimestamp>\n", 
                clm_res->notification_buffer.notification[ndx].clusterNode.bootTimestamp);
        fprintf(fp, "            <initialViewNumber>%lld</initialViewNumber>\n",
                clm_res->notification_buffer.notification[ndx].clusterNode.initialViewNumber);
        fprintf(fp, "        </SAFNode>\n");
    }

    fprintf(fp, "    </SAFNodeList>\n");
    fprintf(fp, "</SAFCluster>\n");
    reply->status = SA_AIS_OK;
    fclose(fp);    
}

void
ais_test_daemon_handle_incoming_client_message(int client_connection_fd,
                                               void *void_request)
{
    ais_test_clm_request_t *request = (ais_test_clm_request_t *)void_request;
    ais_test_clm_reply_t *reply;

    if (NULL == request) {
        ais_test_abort("Invalid (NULL) request\n");
    }

    reply = (ais_test_clm_reply_t *)malloc(sizeof(ais_test_clm_reply_t));
    memset(reply, 0, sizeof(ais_test_clm_reply_t));

    switch(request->op) {
        case AIS_TEST_CLM_REQUEST_CREATE_TEST_RESOURCE:
            ais_test_daemon_handle_create_test_res_request(request, reply);
            break;
        case AIS_TEST_CLM_REQUEST_INITALIZE:
            ais_test_daemon_handle_init_request(request, reply);
            break;
        case AIS_TEST_CLM_REQUEST_SELECTION_OBJECT_GET:
            ais_test_daemon_handle_selection_object_request(request, reply);
            break;
        case AIS_TEST_CLM_REQUEST_FINALIZE:
            ais_test_daemon_handle_finalize_request(request, reply);
            break;
        case AIS_TEST_CLM_REQUEST_DISPATCH:
            ais_test_daemon_handle_dispatch_request(request, reply);
            break;
        case AIS_TEST_CLM_REQUEST_CLUSTER_NODE_GET:
            ais_test_daemon_handle_cluster_node_get_request(request, reply);
            break;
        case AIS_TEST_CLM_REQUEST_CLUSTER_NODE_GET_ASYNC:
            ais_test_daemon_handle_cluster_node_get_async_request(request, 
                                                                  reply);
            break;
        case AIS_TEST_CLM_REQUEST_CLUSTER_NODE_GET_CALLBACK_COUNT:
            ais_test_daemon_handle_cluster_node_get_cb_count_request(request, 
                                                                     reply);
            break;
        case AIS_TEST_CLM_REQUEST_CLUSTER_NODE_GET_ASYNC_INVOCATION:
            ais_test_daemon_handle_cluster_node_get_async_invocation_request(
                request, reply);
            break;
        case AIS_TEST_CLM_REQUEST_CLUSTER_TRACK:
            ais_test_daemon_handle_cluster_track_request(request, reply);
            break;
        case AIS_TEST_CLM_REQUEST_CLUSTER_TRACK_STOP:
            ais_test_daemon_handle_cluster_track_stop_request(request, reply);
            break;
        case AIS_TEST_CLM_REQUEST_CLUSTER_TRACK_CALLBACK_COUNT:
            ais_test_daemon_handle_cluster_track_cb_count_request(request, 
                                                                  reply);
            break;
        case AIS_TEST_CLM_REQUEST_DISPLAY_LAST_NOTIFICATION_BUFFER:
            ais_test_daemon_handle_display_last_notification_buffer_request(
                request, reply);
            break;
        default:
            ais_test_abort("Daemon received request with unknown op %d\n",
                           request->op);
    }
    free(request);

    ais_test_send_reply(client_connection_fd, 
                        reply, sizeof(ais_test_clm_reply_t));
    free(reply);
}

void ais_test_daemon_add_clm_resource_to_fdset(gpointer data,
                                                gpointer user_data)
{
    clm_resource_t *clm_res;
    fd_set_key_t *set_key = (fd_set_key_t *)user_data;

    if (NULL == data) {
        return;
    }

    clm_res = (clm_resource_t *)data;
    if (clm_res->thread_id > 0) {
        /* It will have its own dispatch thread */
        return;
    }

    if (clm_res->selection_object > 0) {
        FD_SET(clm_res->selection_object, set_key->set);
        if (clm_res->selection_object > set_key->largest_fd) {
            set_key->largest_fd = clm_res->selection_object;
        }
    }
    return;
}

void
ais_test_daemon_handle_incoming_clm_message(gpointer data, gpointer user_data)
{
    clm_resource_t *clm_res = data;
    fd_set *fd_mask = (fd_set *)user_data;
    SaAisErrorT err;

    if (NULL == data) {
        return;
    }

    if (!FD_ISSET(clm_res->selection_object, fd_mask)) {
        return;
    }

    ais_test_log("Incoming request on clm selection fd %d.  "
                 "Calling saClmDispatch\n",
                 clm_res->selection_object);
    err = saClmDispatch(clm_res->clm_handle, clm_res->dispatch_flags);
    if (SA_AIS_OK != err) {
        ais_test_log("Error %s performing saClmDispatch\n",
                     get_error_string(err));
    }
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

    g_list_foreach(clm_list, ais_test_daemon_add_clm_resource_to_fdset,
                   &set_key);
    *max_fd = set_key.largest_fd;
}

void
ais_test_daemon_check_fds(
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    g_list_foreach(clm_list,
                   ais_test_daemon_handle_incoming_clm_message,
                   read_fd_set);
}

SaAisErrorT
ais_test_client_handle_create_test_res_request(
    int fd,
    ais_test_clm_request_t *request)
{
    ais_test_clm_reply_t *reply;
    SaAisErrorT status;
 
    reply = ais_test_send_request(fd, request, 
                                  sizeof(ais_test_clm_request_t),
                                  sizeof(ais_test_clm_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    if (SA_AIS_OK == status) {
        ais_test_log("Resource ID=%d\n", reply->clm_resource_id);
    }
    free(reply);
    return(status);
}

SaAisErrorT
ais_test_client_handle_init_request(int fd,
                                    ais_test_clm_request_t *request,
                                    int clm_resource_id,
                                    SaVersionT *sa_version,
                                    int cluster_node_get_cb_flag,
                                    int cluster_track_cb_flag,
                                    SaDispatchFlagsT dispatch_flags,
                                    int null_clm_handle_flag,
                                    int null_callbacks_flag,
                                    int null_version_flag)
{
    ais_test_clm_reply_t *reply;
    SaAisErrorT status;
 
    request->clm_resource_id = clm_resource_id;
    request->sa_version = *sa_version;
    request->cluster_node_get_cb_flag = cluster_node_get_cb_flag;
    request->cluster_track_cb_flag = cluster_track_cb_flag;
    request->dispatch_flags = dispatch_flags;
    request->null_clm_handle_flag = null_clm_handle_flag;
    request->null_callbacks_flag = null_callbacks_flag;
    request->null_version_flag = null_version_flag;
    reply = ais_test_send_request(fd, request, 
                                  sizeof(ais_test_clm_request_t),
                                  sizeof(ais_test_clm_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }
    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_selection_object_request(int fd,
                                             ais_test_clm_request_t *request,
                                             int clm_resource_id,
                                             int null_selection_object_flag)
{
    ais_test_clm_reply_t *reply;
    SaAisErrorT status;
 
    request->clm_resource_id = clm_resource_id;
    request->null_selection_object_flag = null_selection_object_flag;
    reply = ais_test_send_request(fd, request, 
                                  sizeof(ais_test_clm_request_t),
                                  sizeof(ais_test_clm_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_cluster_node_get_request(
    int fd,
    ais_test_clm_request_t *request,
    int clm_resource_id,
    SaClmNodeIdT node_id, 
    SaTimeT timeout,
    int null_cluster_node_flag)
{
    ais_test_clm_reply_t *reply;
    SaAisErrorT status;
 
    request->clm_resource_id = clm_resource_id;
    request->node_id = node_id;
    request->timeout = timeout;
    request->null_cluster_node_flag = null_cluster_node_flag;
    reply = ais_test_send_request(fd, request, 
                                  sizeof(ais_test_clm_request_t),
                                  sizeof(ais_test_clm_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_cluster_node_get_async_request(
    int fd,
    ais_test_clm_request_t *request,
    int clm_resource_id,
    SaInvocationT invocation,
    SaClmNodeIdT node_id)
{
    ais_test_clm_reply_t *reply;
    SaAisErrorT status;
 
    request->clm_resource_id = clm_resource_id;
    request->invocation = invocation;
    request->node_id = node_id;
    reply = ais_test_send_request(fd, request, 
                                  sizeof(ais_test_clm_request_t),
                                  sizeof(ais_test_clm_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_cluster_node_get_cb_count_request(
    int fd,
    ais_test_clm_request_t *request,
    int clm_resource_id)
{
    ais_test_clm_reply_t *reply;
    SaAisErrorT status;
 
    request->clm_resource_id = clm_resource_id;
    reply = ais_test_send_request(fd, request, 
                                  sizeof(ais_test_clm_request_t),
                                  sizeof(ais_test_clm_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    if (SA_AIS_OK == status) {
        ais_test_log("Cluster Node Get Callback Count=%d\n",
                     reply->cluster_node_get_callback_count);
    }

    free(reply);
    return(status);
}

int
ais_test_client_handle_cluster_node_get_async_invocation_request(
    int fd,
    ais_test_clm_request_t *request,
    int clm_resource_id)
{
    ais_test_clm_reply_t *reply;
    SaAisErrorT status;
 
    request->clm_resource_id = clm_resource_id;
    reply = ais_test_send_request(fd, request, 
                                  sizeof(ais_test_clm_request_t),
                                  sizeof(ais_test_clm_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    if (SA_AIS_OK == status) {
        ais_test_log("Cluster Node Get Async Invocation=%d\n",
                     reply->cluster_node_get_async_invocation);
    }

    free(reply);
    return(status);
}

int
ais_test_client_handle_cluster_track_request(
    int fd,
    ais_test_clm_request_t *request,
    int clm_resource_id,
    SaUint8T track_flags, 
    SaUint32T number_of_items,
    int null_notification_buffer_flag,
    int null_cluster_notification_flag)
{
    ais_test_clm_reply_t *reply;
    SaAisErrorT status;
 
    request->clm_resource_id = clm_resource_id;
    request->track_flags = track_flags;
    request->number_of_items = number_of_items;
    request->null_notification_buffer_flag = null_notification_buffer_flag;
    request->null_cluster_notification_flag = null_cluster_notification_flag;
    reply = ais_test_send_request(fd, request, 
                                  sizeof(ais_test_clm_request_t),
                                  sizeof(ais_test_clm_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_cluster_track_stop_request(
    int fd,
    ais_test_clm_request_t *request,
    int clm_resource_id)
{
    ais_test_clm_reply_t *reply;
    SaAisErrorT status;
 
    request->clm_resource_id = clm_resource_id;
    reply = ais_test_send_request(fd, request, 
                                  sizeof(ais_test_clm_request_t),
                                  sizeof(ais_test_clm_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_cluster_track_cb_count_request(
    int fd,
    ais_test_clm_request_t *request,
    int clm_resource_id)
{
    ais_test_clm_reply_t *reply;
    SaAisErrorT status;
 
    request->clm_resource_id = clm_resource_id;
    reply = ais_test_send_request(fd, request, 
                                  sizeof(ais_test_clm_request_t),
                                  sizeof(ais_test_clm_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    if (SA_AIS_OK == status) {
        ais_test_log("Cluster Track Callback Count=%d\n",
                     reply->cluster_track_callback_count);
    }

    free(reply);
    return(status);
}

int
ais_test_client_handle_resource_finalize_request(
    int fd,
    ais_test_clm_request_t *request, 
    int clm_resource_id)
{
    ais_test_clm_reply_t *reply;
    SaAisErrorT status;
 
    request->clm_resource_id = clm_resource_id;
    reply = ais_test_send_request(fd, request, 
                                  sizeof(ais_test_clm_request_t),
                                  sizeof(ais_test_clm_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_dispatch_request(int fd,
                                        ais_test_clm_request_t *request,
                                        int clm_resource_id,
                                        SaDispatchFlagsT dispatch_flags)
{
    ais_test_clm_reply_t *reply;
    SaAisErrorT status;
 
    request->clm_resource_id = clm_resource_id;
    request->dispatch_flags = dispatch_flags;
    reply = ais_test_send_request(fd, request, 
                                  sizeof(ais_test_clm_request_t),
                                  sizeof(ais_test_clm_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }
    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_display_last_notification_buffer_request(
    int fd,
    ais_test_clm_request_t *request,
    int clm_resource_id,
    const char *xml_file)
{
    ais_test_clm_reply_t *reply;
    SaAisErrorT status;
 
    request->clm_resource_id = clm_resource_id;
    strcpy(request->xml_file, xml_file);
    reply = ais_test_send_request(fd, request, 
                                  sizeof(ais_test_clm_request_t),
                                  sizeof(ais_test_clm_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }
    status = reply->status;
    free(reply);
    return(status);
}

#define HELP_OPTION 1
#define SOCKET_FILE_OPTION 4
#define RUN_DIR_OPTION 5
#define LOG_FILE_OPTION 6
#define PID_FILE_OPTION 7
#define OP_NAME_OPTION 8
#define XML_FILE_OPTION 9
#define SET_CLUSTER_NODE_GET_CB_OPTION 11
#define SET_CLUSTER_TRACK_CB_OPTION 12
#define INVOCATION_OPTION 13
#define NODE_ID_OPTION 14
#define CLM_RESOURCE_ID_OPTION 15
#define TIMEOUT_OPTION 16
#define TRACK_CURRENT_OPTION 17
#define TRACK_CHANGES_OPTION 18
#define TRACK_CHANGES_ONLY_OPTION 19
#define NULL_NOTIFICATION_BUFFER_OPTION 20 
#define INVALID_TRACK_FLAGS_OPTION 21
#define NUMBER_OF_ITEMS_OPTION 22
#define DISPATCH_FLAGS_OPTION 23
#define NULL_CLM_HANDLE_OPTION 24
#define NULL_CALLBACKS_OPTION 25
#define NULL_VERSION_OPTION 26
#define NULL_CLUSTER_NODE_OPTION 27
#define NULL_CLUSTER_NOTIFICATION_OPTION 28
#define NULL_SELECTION_OBJECT_OPTION 29

#define VERSION_RELEASE_CODE_OPTION 30
#define VERSION_MAJOR_OPTION 31
#define VERSION_MINOR_OPTION 32

int
saftest_driver_client_main(int argc, char **argv,
                           void *first_request, int first_request_length)
{
    SaAisErrorT         status = 255;
    int                 socket_file_flag = 0;
    int                 run_dir_flag = 0;
    int                 log_file_flag = 0;
    int                 pid_file_flag = 0;
    int                 op_name_flag = 0;
    int                 xml_file_flag = 0;
    int                 cluster_node_get_cb_flag = 0;
    int                 cluster_track_cb_flag = 0;
    int                 clm_resource_id_flag = 0;
    int     	        verbose_flag = 0;
    int     	        dispatch_type_flag = 0;
    int     	        node_id_flag = 0;
    int     	        invocation_flag = 0;
    int     	        track_current_flag = 0;
    int     	        track_changes_flag = 0;
    int     	        track_changes_only_flag = 0;
    int     	        invalid_track_flags_flag = 0;
    int     	        number_of_items_flag = 0;
    int     	        null_notification_buffer_flag = 0;
    int     	        null_clm_handle_flag = 0;
    int     	        null_callbacks_flag = 0;
    int     	        null_version_flag = 0;
    int     	        null_cluster_node_flag = 0;
    int     	        null_selection_object_flag = 0;
    int     	        null_cluster_notification_flag = 0;
    int                 version_release_code_flag = 0;
    int                 version_major_flag = 0;
    int                 version_minor_flag = 0;
    int                 timeout_flag = 0;
    char                run_path[BUF_SIZE];
    char                pid_file[BUF_SIZE];
    char                log_file[BUF_SIZE];
    char                xml_file[BUF_SIZE];
    char                socket_file[BUF_SIZE];
    char             	op_name[BUF_SIZE];
    ais_test_clm_request_op_t op;
    int                 clm_resource_id = 0;
    SaVersionT          sa_version;
    SaDispatchFlagsT    dispatch_flags = 0;
    SaInvocationT       invocation;
    SaClmNodeIdT        node_id;
    SaTimeT             timeout;
    SaUint8T            track_flags = 0;
    SaUint32T           number_of_items = 0;
    int next_option = 0;
    ais_test_clm_request_t *request;
    int client_fd;

    const struct option long_options[] = {
        { "help",     0, NULL, HELP_OPTION},
        { "socket-file", 1, NULL, SOCKET_FILE_OPTION},
        { "run-dir", 1, NULL, RUN_DIR_OPTION},
        { "log-file", 1, NULL, LOG_FILE_OPTION},
        { "pid-file", 1, NULL, PID_FILE_OPTION},
        { "xml-file", 1, NULL, XML_FILE_OPTION},
        { "op", 1, NULL, OP_NAME_OPTION},
        { "set-cluster-node-get-cb", 0, NULL, SET_CLUSTER_NODE_GET_CB_OPTION},
        { "set-cluster-track-cb", 0, NULL, SET_CLUSTER_TRACK_CB_OPTION},
        { "resource-id", 1, NULL, CLM_RESOURCE_ID_OPTION},
        { "invocation", 1, NULL, INVOCATION_OPTION},
        { "node-id", 1, NULL, NODE_ID_OPTION},
        { "timeout", 1, NULL, TIMEOUT_OPTION},
        { "dispatch-flags", 1, NULL, DISPATCH_FLAGS_OPTION},
        { "track-current", 0, NULL, TRACK_CURRENT_OPTION},
        { "track-changes", 0, NULL, TRACK_CHANGES_OPTION},
        { "track-changes-only", 0, NULL, TRACK_CHANGES_ONLY_OPTION},
        { "invalid-track-flags", 0, NULL, INVALID_TRACK_FLAGS_OPTION},
        { "number-of-items", 1, NULL, NUMBER_OF_ITEMS_OPTION},
        { "null-notification-buffer", 0, NULL, NULL_NOTIFICATION_BUFFER_OPTION},
        { "null-cluster-notification", 0, NULL, NULL_CLUSTER_NOTIFICATION_OPTION},
        { "null-clm-handle", 0, NULL, NULL_CLM_HANDLE_OPTION},
        { "null-callbacks", 0, NULL, NULL_CALLBACKS_OPTION},
        { "null-version", 0, NULL, NULL_VERSION_OPTION},
        { "null-selection-object", 0, NULL, NULL_SELECTION_OBJECT_OPTION},
        { "null-cluster-node", 0, NULL, NULL_CLUSTER_NODE_OPTION},
        { "version-release-code", 1, NULL, VERSION_RELEASE_CODE_OPTION},
        { "version-major-code", 1, NULL, VERSION_MAJOR_OPTION},
        { "version-minor-code", 1, NULL, VERSION_MINOR_OPTION},
        { NULL,       0, NULL, 0   }   /* Required at end of array.  */
    };

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
            case XML_FILE_OPTION:
                if (xml_file_flag) {
                    usage();
                }
                xml_file_flag++;
                strcpy(xml_file, optarg);
                break;
            case SET_CLUSTER_NODE_GET_CB_OPTION:
                if (cluster_node_get_cb_flag) {
                    usage();
                }
                cluster_node_get_cb_flag++;
                break;
            case SET_CLUSTER_TRACK_CB_OPTION:
                if (cluster_track_cb_flag) {
                    usage();
                }
                cluster_track_cb_flag++;
                break;
            case CLM_RESOURCE_ID_OPTION:
                if (clm_resource_id_flag) {
                    usage();
                }
                clm_resource_id_flag++;
                clm_resource_id = atoi(optarg);
                break;
            case INVOCATION_OPTION:
                if (invocation_flag) {
                    usage();
                }
                invocation_flag++;
                invocation = atoi(optarg);
                break;
            case NODE_ID_OPTION:
                if (node_id_flag) {
                    usage();
                }
                node_id_flag++;
                if (0 == strcmp(optarg, "SA_CLM_LOCAL_NODE_ID")) {
                    node_id = SA_CLM_LOCAL_NODE_ID;
                } else {
                    node_id = atoi(optarg);
                }
                break;
            case TIMEOUT_OPTION:
                if (timeout_flag) {
                    usage();
                }
                timeout_flag++;
                timeout = atoi(optarg);
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
            case NUMBER_OF_ITEMS_OPTION:
                if (number_of_items_flag) {
                    usage();
                }
                number_of_items_flag++;
                number_of_items_flag = atoi(optarg);
                break;
            case TRACK_CURRENT_OPTION:
                if (track_current_flag) {
                    usage();
                }
                track_current_flag++;
                track_flags |= SA_TRACK_CURRENT;
                break;
            case TRACK_CHANGES_OPTION:
                if (track_changes_flag) {
                    usage();
                }
                track_changes_flag++;
                track_flags |= SA_TRACK_CHANGES;
                break;
            case TRACK_CHANGES_ONLY_OPTION:
                if (track_changes_only_flag) {
                    usage();
                }
                track_changes_only_flag++;
                track_flags |= SA_TRACK_CHANGES_ONLY;
                break;
            case INVALID_TRACK_FLAGS_OPTION:
                if (invalid_track_flags_flag) {
                    usage();
                }
                invalid_track_flags_flag++;
                track_flags = -1;
                break;
            case NULL_NOTIFICATION_BUFFER_OPTION:
                if (null_notification_buffer_flag) {
                    usage();
                }
                null_notification_buffer_flag++;
                break;
            case NULL_CLM_HANDLE_OPTION:
                if (null_clm_handle_flag) {
                    usage();
                }
                null_clm_handle_flag++;
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
            case NULL_SELECTION_OBJECT_OPTION:
                if (null_selection_object_flag) {
                    usage();
                }
                null_selection_object_flag++;
                break;
            case NULL_CLUSTER_NODE_OPTION:
                if (null_cluster_node_flag) {
                    usage();
                }
                null_cluster_node_flag++;
                break;
            case NULL_CLUSTER_NOTIFICATION_OPTION:
                if (null_cluster_notification_flag) {
                    usage();
                }
                null_cluster_notification_flag++;
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
    saftest_driver_client_init(run_path);

    ais_test_uds_connect(&client_fd, socket_file);
    ais_test_send_request(client_fd, first_request, first_request_length, 0);

    op = ais_test_map_string_to_op(op_name);

    request = (ais_test_clm_request_t *)
              malloc(sizeof(ais_test_clm_request_t));
    memset(request, 0, sizeof(ais_test_clm_request_t));
    request->op = op;
    request->requestor_pid = getpid();
    switch(op) {
        case AIS_TEST_CLM_REQUEST_CREATE_TEST_RESOURCE:
            status = 
                ais_test_client_handle_create_test_res_request(
                    client_fd, request);
            break;
        case AIS_TEST_CLM_REQUEST_INITALIZE:
            if (!clm_resource_id_flag || !dispatch_type_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_init_request(client_fd,
                                                    request,
                                                    clm_resource_id,
                                                    &sa_version,
                                                    cluster_node_get_cb_flag,
                                                    cluster_track_cb_flag,
                                                    dispatch_flags,
                                                    null_clm_handle_flag,
                                                    null_callbacks_flag,
                                                    null_version_flag);
            break;
        case AIS_TEST_CLM_REQUEST_SELECTION_OBJECT_GET:
            if (!clm_resource_id_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_selection_object_request(
                    client_fd, request, clm_resource_id,
                    null_selection_object_flag);
            break;
        case AIS_TEST_CLM_REQUEST_FINALIZE:
            if (!clm_resource_id_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_resource_finalize_request(
                    client_fd, request, clm_resource_id);
            break;
        case AIS_TEST_CLM_REQUEST_DISPATCH:
            /*
             * Can specify either a specific handle or a clm_resource_id
             */
            if (!clm_resource_id_flag || !dispatch_type_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_dispatch_request(
                    client_fd, request, clm_resource_id, dispatch_flags);
            break;
        case AIS_TEST_CLM_REQUEST_CLUSTER_NODE_GET:
            if (!clm_resource_id_flag || !node_id_flag || 
                !timeout_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_cluster_node_get_request(
                    client_fd, request, clm_resource_id, node_id, 
                    timeout, null_cluster_node_flag);
            break;
        case AIS_TEST_CLM_REQUEST_CLUSTER_NODE_GET_ASYNC:
            if (!clm_resource_id_flag || !node_id_flag || 
                !invocation_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_cluster_node_get_async_request(
                    client_fd, request, clm_resource_id, 
                    invocation, node_id);
            break;
        case AIS_TEST_CLM_REQUEST_CLUSTER_NODE_GET_CALLBACK_COUNT:
            if (!clm_resource_id_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_cluster_node_get_cb_count_request(
                    client_fd, request, clm_resource_id);
            break;
        case AIS_TEST_CLM_REQUEST_CLUSTER_NODE_GET_ASYNC_INVOCATION:
            if (!clm_resource_id_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_cluster_node_get_async_invocation_request(
                    client_fd, request, clm_resource_id);
            break;
        case AIS_TEST_CLM_REQUEST_CLUSTER_TRACK:
            if (!clm_resource_id_flag) {
                usage();
            }
            if (invalid_track_flags_flag && 
                (track_current_flag || track_changes_flag ||
                 track_changes_only_flag)) {
                usage();
            }
            status = 
                ais_test_client_handle_cluster_track_request(
                    client_fd, request, clm_resource_id, 
                    track_flags, number_of_items,
                    null_notification_buffer_flag, 
                    null_cluster_notification_flag);
            break;
        case AIS_TEST_CLM_REQUEST_CLUSTER_TRACK_STOP:
            if (!clm_resource_id_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_cluster_track_stop_request(
                    client_fd, request, clm_resource_id);
            break;
        case AIS_TEST_CLM_REQUEST_CLUSTER_TRACK_CALLBACK_COUNT:
            if (!clm_resource_id_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_cluster_track_cb_count_request(
                    client_fd, request, clm_resource_id);
            break;
        case AIS_TEST_CLM_REQUEST_DISPLAY_LAST_NOTIFICATION_BUFFER:
            if (!clm_resource_id_flag || !xml_file_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_display_last_notification_buffer_request(
                    client_fd, request, clm_resource_id, xml_file);
            break;
        default:
            ais_test_abort("Client received request with unknown op %s\n",
                           op_name);
    }
    free(request);
    close(client_fd);
    if (verbose_flag) {
        ais_test_log("Exit status for %s request is %s\n",
                    op_name, get_error_string(status));
    }

    exit(status);
}
