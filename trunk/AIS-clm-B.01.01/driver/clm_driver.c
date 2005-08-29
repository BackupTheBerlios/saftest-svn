/**********************************************************************
 *
 *	I N C L U D E S
 * Was 1755, down to 922
 **********************************************************************/
#include "saftest_driver_lib_utils.h"
#include "saftest_driver.h"
#include "saftest_log.h"
#include "saftest_comm.h"
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

const char *get_library_id()
{
    return "CLM";
}

void saftest_daemon_init(FILE *log_fp)
{
    assert(NULL != log_fp);
    saftest_log_set_fp(log_fp);
}

SaClmNodeIdT get_node_id_from_string(const char *node_id_str)
{
    if (0 == strcmp("SA_CLM_LOCAL_NODE_ID", node_id_str)) {
        return SA_CLM_LOCAL_NODE_ID;
    }
    return atoi(node_id_str);
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
    saftest_log("Added a clm resource with id %d\n", res->clm_resource_id);
    return(res);
}

void
delete_clm_resource(clm_resource_t *res)
{
    saftest_log("Deleting clm resource with id %d\n", res->clm_resource_id);
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
lookup_clm_resource_from_request(saftest_msg_t *request)
{
    clm_resource_t *res;

    res = lookup_clm_resource(
                  saftest_msg_get_ubit32_value(request, "CLM_RESOURCE_ID"));
    if (NULL == res) {
        saftest_abort("Unknown resource id %d\n",
                      saftest_msg_get_ubit32_value(request, "CLM_RESOURCE_ID"));
    }
    return(res);
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
saftest_daemon_cluster_node_get_callback(SaInvocationT invocation,
                                          const SaClmClusterNodeT *clusterNode,
                                          SaAisErrorT error)
{
    clm_resource_t *clm_res = NULL;

    saftest_log("Cluster Node Get Callback for invocation %lld\n", invocation);
    clm_res = lookup_clm_resource_by_invocation(invocation);
    if (NULL == clm_res) {
        saftest_abort("Unknown invocation id %d\n",
                       invocation);
    }
    clm_res->cluster_node_get_callback_count += 1;
}

void
saftest_daemon_cluster_track_callback(
    const SaClmClusterNotificationBufferT *notificationBuffer,
    SaUint32T numberOfMembers,
    SaAisErrorT error)
{
    clm_resource_t *clm_res = NULL;

    clm_res = lookup_clm_resource_with_track_callback();
    if (NULL == clm_res) {
        saftest_abort("Unable to find a clm_resource with track callback\n");
    }
    saftest_log("Received a cluster track callback\n");
    clm_res->cluster_track_callback_count += 1;
    /* This is a vicious, dirty hack that causes memory leaks and stuff */
    clm_res->notification_buffer = *notificationBuffer;
}

static void *
saftest_daemon_dispatch_thread(void *arg)
{
    clm_resource_t *clm_res = (clm_resource_t *)arg;
    SaAisErrorT err;
    
    err = saClmDispatch(clm_res->clm_handle, SA_DISPATCH_BLOCKING);
    if (err != SA_AIS_OK) {
        saftest_abort("Error %s calling "
                       "saClmDispatch(SA_DISPATCH_BLOCKING)\n",
                       get_error_string(err));
    }
    return(NULL);
}

void
saftest_daemon_handle_create_test_res_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_resource_t *clm_res;
    
    saftest_log("Received a create test resource request.\n");

    clm_res = add_clm_resource();

    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op,
                                        SA_AIS_OK);
    saftest_msg_set_ubit32_value((*reply), "CLM_RESOURCE_ID",
                                 clm_res->clm_resource_id);
}

void
saftest_daemon_handle_init_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_resource_t *clm_res;
    int err;
    SaClmHandleT *handle = NULL;
    SaClmCallbacksT *callbacks = NULL;
    SaVersionT *version = NULL;
    SaAisErrorT status;
    char *releaseCodeStr;
    char releaseCode;

    releaseCodeStr = saftest_msg_get_str_value(request,
                                               "VERSION_RELEASE_CODE");
    releaseCode = releaseCodeStr[0];
    saftest_log("Received an init request from for id %d "
                 "release code=%c, majorVersion=%d, minorVersion=%d\n",
                 saftest_msg_get_ubit32_value(request, "CLM_RESOURCE_ID"),
                 releaseCode,
                 saftest_msg_get_ubit32_value(request, "VERSION_MAJOR"),
                 saftest_msg_get_ubit32_value(request, "VERSION_MINOR"));

    clm_res = lookup_clm_resource_from_request(request);
    clm_res->clm_callbacks.saClmClusterNodeGetCallback = NULL;
    clm_res->clm_callbacks.saClmClusterTrackCallback = NULL;

    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_CLM_HANDLE"))) {
        handle = &clm_res->clm_handle;
    }
    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_CALLBACKS"))) {
        callbacks = &clm_res->clm_callbacks;
        if (0 == strcmp("TRUE",
                        saftest_msg_get_str_value(request,
                                                  "CLUSTER_NODE_GET_CB"))) {
            clm_res->clm_callbacks.saClmClusterNodeGetCallback =
                saftest_daemon_cluster_node_get_callback;
        }
        if (0 == strcmp("TRUE",
                        saftest_msg_get_str_value(request,
                                                  "CLUSTER_TRACK_CB"))) {
            clm_res->clm_callbacks.saClmClusterTrackCallback =
                saftest_daemon_cluster_track_callback;
        }
    }
    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_VERSION"))) {
        version = &clm_res->version;
        clm_res->version.releaseCode = releaseCode;
        clm_res->version.majorVersion =
            saftest_msg_get_ubit8_value(request, "VERSION_MAJOR");
        clm_res->version.minorVersion =
            saftest_msg_get_ubit8_value(request, "VERSION_MINOR");
    }

    clm_res->dispatch_flags =
        saftest_daemon_get_dispatch_flags(
                   saftest_msg_get_str_value(request, "DISPATCH_FLAGS"));

    status = saClmInitialize(handle, callbacks, version);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
    if (SA_AIS_OK == status) {
        if (clm_res->dispatch_flags == SA_DISPATCH_BLOCKING) {
            saftest_log("Starting new dispatch thread\n");
            err = pthread_create(&clm_res->thread_id, NULL,
                                 saftest_daemon_dispatch_thread,
                                 (void*)clm_res);
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
    clm_resource_t *clm_res = NULL;
    SaSelectionObjectT *selection_object = NULL;
    SaAisErrorT status;

    saftest_log("Received a select object get request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_RESOURCE_ID"));
    clm_res = lookup_clm_resource_from_request(request);

    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request,
                                              "NULL_SELECTION_OBJECT"))) {
        selection_object = &clm_res->selection_object;
    }
    status = saClmSelectionObjectGet(clm_res->clm_handle, selection_object);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_dispatch_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_resource_t *clm_res = NULL;
    SaAisErrorT status;
    SaDispatchFlagsT dispatch_flags;

    saftest_log("Received a dispatch request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_RESOURCE_ID"));
    clm_res = lookup_clm_resource_from_request(request);

    dispatch_flags =
        saftest_daemon_get_dispatch_flags(
                   saftest_msg_get_str_value(request, "DISPATCH_FLAGS"));
    status = saClmDispatch(clm_res->clm_handle, dispatch_flags);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_resource_finalize_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_resource_t *clm_res = NULL;
    SaAisErrorT status;

    saftest_log("Received a finalize request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_RESOURCE_ID"));
    clm_res = lookup_clm_resource_from_request(request);

    status = saClmFinalize(clm_res->clm_handle);
    clm_res->selection_object = 0;
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_cluster_node_get_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_resource_t *clm_res = NULL;
    SaAisErrorT status;
    SaClmNodeIdT node_id;

    SaClmClusterNodeT cluster_node;
    SaClmClusterNodeT *cluster_node_ptr = NULL;

    saftest_log("Received a cluster node get request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_RESOURCE_ID"));
    clm_res = lookup_clm_resource_from_request(request);
    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_CLUSTER_NODE"))) {
        cluster_node_ptr = &cluster_node;
    }
    /*
     * !!! Need to address this timeout problem.  What is a timeout of 0
     * supposed to represent?
     */
    node_id = get_node_id_from_string(
                      saftest_msg_get_str_value(request, "NODE_ID"));
    status = saClmClusterNodeGet(clm_res->clm_handle, 
                                 node_id, 
                                 saftest_msg_get_ubit32_value(request, 
                                                              "TIMEOUT"),
                                 cluster_node_ptr);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_cluster_node_get_async_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_resource_t *clm_res = NULL;
    SaAisErrorT status;
    SaClmNodeIdT node_id;

    saftest_log("Received a cluster node get async request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_RESOURCE_ID"));
    clm_res = lookup_clm_resource_from_request(request);

    clm_res->cluster_node_get_async_invocation = 
                      saftest_msg_get_ubit64_value(request, "INVOCATION");
    node_id = get_node_id_from_string(
                      saftest_msg_get_str_value(request, "NODE_ID"));
    status = saClmClusterNodeGetAsync(
                      clm_res->clm_handle, 
                      saftest_msg_get_ubit64_value(request, "INVOCATION"),
                      node_id);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_cluster_node_get_cb_count_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_resource_t *clm_res = NULL;

    saftest_log("Received a cluster node get cb count request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_RESOURCE_ID"));
    clm_res = lookup_clm_resource_from_request(request);

    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        SA_AIS_OK);
    saftest_msg_set_ubit32_value(*reply, 
                                 "NODE_GET_CALLBACK_COUNT",
                                 clm_res->cluster_node_get_callback_count);
}

void
saftest_daemon_handle_cluster_node_get_async_invocation_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_resource_t *clm_res = NULL;

    saftest_log("Received a cluster node get async invocation request for id "
                 "%d\n",
                saftest_msg_get_ubit32_value(request, "CLM_RESOURCE_ID"));
    clm_res = lookup_clm_resource_from_request(request);

    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        SA_AIS_OK);
    saftest_msg_set_ubit32_value(*reply, 
                                 "NODE_GET_ASYNC_INVOCATION",
                                 clm_res->cluster_node_get_async_invocation);
}

void
saftest_daemon_handle_cluster_track_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_resource_t *clm_res = NULL;
    SaClmClusterNotificationBufferT *buffer = NULL;
    SaAisErrorT status;
    SaUint8T track_flags = 0;

    saftest_log("Received a cluster track request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_RESOURCE_ID"));
    clm_res = lookup_clm_resource_from_request(request);

    if (0 == strcmp("TRUE",
                    saftest_msg_get_str_value(request,
                                              "TRACK_CURRENT"))) {
        track_flags |= SA_TRACK_CURRENT;
    }
    if (0 == strcmp("TRUE",
                    saftest_msg_get_str_value(request,
                                              "TRACK_CHANGES"))) {
        track_flags |= SA_TRACK_CHANGES;
    }
    if (0 == strcmp("TRUE",
                    saftest_msg_get_str_value(request,
                                              "TRACK_CHANGES_ONLY"))) {
        track_flags |= SA_TRACK_CHANGES_ONLY;
    }
    if (0 == strcmp("TRUE",
                    saftest_msg_get_str_value(request,
                                              "INVALID_TRACK_FLAGS"))) {
        track_flags = -1;
    }
    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request,
                                              "NULL_NOTIFICATION_BUFFER"))) {
        buffer = &clm_res->notification_buffer;
        if (0 == 
            strcmp("FALSE", 
                   saftest_msg_get_str_value(request,
                                             "NULL_CLUSTER_NOTIFICATION"))) {
            buffer->numberOfItems = 
                saftest_msg_get_ubit32_value(request, "NUMBER_OF_ITEMS");
            if (0 == buffer->numberOfItems) {
                /* 
                 * This is a special unit test case, setting a pointer for
                 * notification_buffer->notification but setting numberOfItems
                 * to be 0.
                 */
                buffer->notification = (SaClmClusterNotificationT *)buffer;
            } else {
                buffer->notification = 
                    (SaClmClusterNotificationT *)
                    malloc(buffer->numberOfItems * 
                           sizeof(SaClmClusterNotificationT));
                if (NULL == buffer->notification) {
                    saftest_abort("unable to allocate cluster notification array");
                }
            }
        }
    }
    clm_res->track_flags = track_flags;
    status = saClmClusterTrack(clm_res->clm_handle, clm_res->track_flags,
                               buffer);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_cluster_track_stop_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_resource_t *clm_res = NULL;
    SaAisErrorT status;

    saftest_log("Received a cluster track stop request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_RESOURCE_ID"));
    clm_res = lookup_clm_resource_from_request(request);

    status = saClmClusterTrackStop(clm_res->clm_handle);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_cluster_track_cb_count_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_resource_t *clm_res = NULL;

    saftest_log("Received a cluster track count request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_RESOURCE_ID"));
    clm_res = lookup_clm_resource_from_request(request);

    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        SA_AIS_OK);
    saftest_msg_set_ubit32_value(*reply, "CLUSTER_TRACK_CALLBACK_COUNT",
                                 clm_res->cluster_track_callback_count);
}

void 
saftest_daemon_handle_finalize_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_resource_t *clm_res = NULL;
    SaAisErrorT status;

    saftest_log("Received a finalize request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_RESOURCE_ID"));
    clm_res = lookup_clm_resource_from_request(request);

    status = saClmFinalize(clm_res->clm_handle);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        status);
    clm_res->selection_object = 0;
}

void 
saftest_daemon_handle_display_last_notification_buffer_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_resource_t *clm_res = NULL;
    char node_name[SA_MAX_NAME_LENGTH+1];
    FILE *fp = NULL;
    const char *family;
    SaUint32T ndx;
    struct in_addr in_addr;
    char addr_buf[INET6_ADDRSTRLEN];
    const char *xml_file;

    xml_file = saftest_msg_get_str_value(request, "XML_FILE");
    saftest_log("Received a display last notification buffer request for "
                "id %d to XML file %s\n",
                saftest_msg_get_ubit32_value(request, "CLM_RESOURCE_ID"),
                xml_file);
    memset(node_name, 0, sizeof(node_name));
    clm_res = lookup_clm_resource_from_request(request);

    fp = fopen(xml_file, "w+");
    if (NULL == fp) {
        saftest_abort("Unable to open %s for writing\n", xml_file);
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
            saftest_abort("Unknown address family\n");
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
    fclose(fp);    
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        SA_AIS_OK);
}

void saftest_daemon_add_clm_resource_to_fdset(gpointer data,
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
saftest_daemon_handle_incoming_clm_message(gpointer data, gpointer user_data)
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

    saftest_log("Incoming request on clm selection fd %d.  "
                 "Calling saClmDispatch\n",
                 clm_res->selection_object);
    err = saClmDispatch(clm_res->clm_handle, clm_res->dispatch_flags);
    if (SA_AIS_OK != err) {
        saftest_log("Error %s performing saClmDispatch\n",
                     get_error_string(err));
    }
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

    g_list_foreach(clm_list, saftest_daemon_add_clm_resource_to_fdset,
                   &set_key);
    *max_fd = set_key.largest_fd;
}

void
saftest_daemon_check_fds(
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    g_list_foreach(clm_list,
                   saftest_daemon_handle_incoming_clm_message,
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
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");

    status = saftest_reply_msg_get_status(reply);
    if (SA_AIS_OK == status) {
        saftest_log("Resource ID=%d\n", 
                    saftest_msg_get_ubit32_value(reply, 
                                                 "CLM_RESOURCE_ID"));
    }
    status = saftest_reply_msg_get_status(reply);
    saftest_msg_free(&reply);

    return(status);
}

SaAisErrorT
saftest_client_handle_cluster_node_get_cb_count_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply = NULL;
    SaAisErrorT status;
 
    saftest_send_request(fd, get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");

    status = saftest_reply_msg_get_status(reply);
    if (SA_AIS_OK == status) {
        saftest_log("Cluster Node Get Callback Count=%d\n",
                    saftest_msg_get_ubit32_value(reply, 
                                                 "NODE_GET_CALLBACK_COUNT"));
    }
    status = saftest_reply_msg_get_status(reply);
    saftest_msg_free(&reply);

    return(status);
}

SaAisErrorT
saftest_client_handle_cluster_node_get_async_invocation_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply = NULL;
    SaAisErrorT status;
 
    saftest_send_request(fd, get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");

    status = saftest_reply_msg_get_status(reply);
    if (SA_AIS_OK == status) {
        saftest_log("Cluster Node Get Async Invocation=%d\n",
                    saftest_msg_get_ubit32_value(reply, 
                                                 "NODE_GET_ASYNC_INVOCATION"));
    }

    free(reply);
    return(status);
}

SaAisErrorT
saftest_client_handle_cluster_track_cb_count_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply = NULL;
    SaAisErrorT status;
 
    saftest_send_request(fd, get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");

    status = saftest_reply_msg_get_status(reply);
    if (SA_AIS_OK == status) {
        saftest_log("Cluster Track Callback Count=%d\n",
                    saftest_msg_get_ubit32_value(
                        reply, 
                        "CLUSTER_TRACK_CALLBACK_COUNT"));
    }

    free(reply);
    return(status);
}

SAFTEST_MAP_TABLE_BEGIN()
SAFTEST_MAP_TABLE_ENTRY(
    "CREATE_TEST_RESOURCE_REQ", "CREATE_TEST_RESOURCE_REPLY",
    saftest_client_handle_create_test_res_request,
    saftest_daemon_handle_create_test_res_request)

SAFTEST_MAP_TABLE_ENTRY(
    "INITIALIZE_REQ", "INITIALIZE_REPLY",
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
    "CLUSTER_NODE_GET_REQ", "CLUSTER_NODE_GET_REPLY",
     saftest_client_generic_handle_request,
     saftest_daemon_handle_cluster_node_get_request)

SAFTEST_MAP_TABLE_ENTRY(
    "CLUSTER_NODE_GET_ASYNC_REQ", "CLUSTER_NODE_GET_ASYNC_REPLY",
     saftest_client_generic_handle_request,
     saftest_daemon_handle_cluster_node_get_async_request)

SAFTEST_MAP_TABLE_ENTRY(
    "CLUSTER_NODE_GET_CALLBACK_COUNT_REQ",
    "CLUSTER_NODE_GET_CALLBACK_COUNT_REPLY",
     saftest_client_handle_cluster_node_get_cb_count_request,
     saftest_daemon_handle_cluster_node_get_cb_count_request)

SAFTEST_MAP_TABLE_ENTRY(
    "CLUSTER_NODE_GET_ASYNC_INVOCATION_REQ",
    "CLUSTER_NODE_GET_ASYNC_INVOCATION_REPLY",
     saftest_client_handle_cluster_node_get_async_invocation_request,
     saftest_daemon_handle_cluster_node_get_async_invocation_request)

SAFTEST_MAP_TABLE_ENTRY(
    "CLUSTER_TRACK_REQ", "CLUSTER_TRACK_REPLY", 
     saftest_client_generic_handle_request,
     saftest_daemon_handle_cluster_track_request)

SAFTEST_MAP_TABLE_ENTRY(
    "CLUSTER_TRACK_STOP_REQ", "CLUSTER_TRACK_STOP_REPLY",
     saftest_client_generic_handle_request,
     saftest_daemon_handle_cluster_track_stop_request)

SAFTEST_MAP_TABLE_ENTRY(
    "CLUSTER_TRACK_CALLBACK_COUNT_REQ",
    "CLUSTER_TRACK_CALLBACK_COUNT_REPLY",
     saftest_client_handle_cluster_track_cb_count_request,
     saftest_daemon_handle_cluster_track_cb_count_request)

SAFTEST_MAP_TABLE_ENTRY(
    "DISPLAY_LAST_NOTIFICATION_BUFFER_REQ",
    "DISPLAY_LAST_NOTIFICATION_BUFFER_REPLY",
     saftest_client_generic_handle_request,
     saftest_daemon_handle_display_last_notification_buffer_request)
SAFTEST_MAP_TABLE_END()

