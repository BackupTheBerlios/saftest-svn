/**********************************************************************
 *
 *	I N C L U D E S
 *
 **********************************************************************/
#include "saftest_driver_lib_utils.h"
#include "saftest_driver.h"
#include "saftest_log.h"
#include "saftest_comm.h"
#include "string.h"
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

typedef struct clm_session {
    ubit32 clm_session_id;

    SaVersionT version;
    SaTimeT timeout;
    SaDispatchFlagsT dispatch_flags;
    SaClmHandleT clm_handle;
    SaSelectionObjectT selection_object;
    SaUint8T track_flags;
    SaClmCallbacksT clm_callbacks;

    SaInvocationT cluster_node_get_async_invocation;
    char cluster_node_get_callback_xml_file[SAFTEST_STRING_LENGTH + 1];
    char cluster_track_callback_xml_file[SAFTEST_STRING_LENGTH + 1];

    int long_lived;
    int cluster_node_get_callback_count;
    int cluster_track_callback_count;
} clm_session_t;

typedef struct clm_driver_thread {
    int main_thread;
    pthread_t thread_id;
    saftest_list thread_local_session_list;
} clm_driver_thread_t;

saftest_list clm_global_session_list = NULL;
saftest_list clm_thread_data = NULL;

const char *get_library_id();

static clm_driver_thread_t *
add_clm_thread_data()
{
    clm_driver_thread_t *cdt;

    cdt = malloc(sizeof(clm_driver_thread_t));
    assert(NULL != cdt);
    memset(cdt, 0, sizeof(clm_driver_thread_t));

    cdt->thread_id = pthread_self();
    cdt->thread_local_session_list = saftest_list_create();

    saftest_list_element_create(clm_thread_data, cdt);
    return(cdt);
}

static clm_driver_thread_t *
get_current_clm_thread_data()
{
    saftest_list_element element;
    clm_driver_thread_t *cdt;

    for (element = saftest_list_first(clm_thread_data);
         NULL != element;
         element = saftest_list_next(element)) {
        cdt = (clm_driver_thread_t *)element->data;
        if (cdt->thread_id == pthread_self()) {
            return(cdt);
        }
    }
    saftest_abort("We have to have an object representing ourselves "
                  "(self_id = %ld)\n", pthread_self());
    return(NULL);
}

void 
saftest_daemon_init(FILE *log_fp)
{
    assert(NULL != log_fp);
    saftest_log_set_fp(log_fp);

    clm_global_session_list = saftest_list_create();
    clm_thread_data = saftest_list_create();
}

void 
saftest_daemon_thread_init(int main_thread)
{
    clm_driver_thread_t *cdt;

    cdt = add_clm_thread_data();
    cdt->main_thread = main_thread;
}

SaClmNodeIdT get_node_id_from_string(const char *node_id_str)
{
    if (0 == strcmp("SA_CLM_LOCAL_NODE_ID", node_id_str)) {
        return SA_CLM_LOCAL_NODE_ID;
    }
    return atoi(node_id_str);
}

int
get_next_clm_session_id()
{
    static ubit32 next_clm_session_id = 1;
    int ret_id;

    ret_id = next_clm_session_id;
    next_clm_session_id += 1;
    return(ret_id);
}

static int
clm_session_id_comparator(void *data, void *key)
{
    clm_session_t *session = (clm_session_t *)data;
    ubit32 id = (*((ubit32 *)key));
    
    return(session->clm_session_id == id);
}

static int
clm_session_invocation_comparator(void *data, void *key)
{
    clm_session_t *session = (clm_session_t *)data;
    SaInvocationT invocation = (*((SaInvocationT *)key));
    
    return(session->cluster_node_get_async_invocation == invocation);
}

clm_session_t *
add_clm_session(saftest_list session_list)
{
    clm_session_t *session;

    session = malloc(sizeof(clm_session_t));
    assert(NULL != session);
    memset(session, 0, sizeof(clm_session_t));

    saftest_list_element_create(session_list, session);
    session->clm_session_id = get_next_clm_session_id();
    saftest_log("Added a clm session with id %d\n", session->clm_session_id);
    return(session);
}

static saftest_list_element
lookup_clm_session_element(ubit32 clm_session_id)
{
    saftest_list_element element;
    clm_driver_thread_t *cdt;

    element = saftest_list_find(clm_global_session_list,
                                clm_session_id_comparator,
                                &clm_session_id, NULL);
    if (NULL == element) {
        cdt = get_current_clm_thread_data();
        element = saftest_list_find(cdt->thread_local_session_list,
                                    clm_session_id_comparator,
                                    &clm_session_id, NULL);
    }
    saftest_assert(NULL != element,
                   "Attempt to find unknown session");
    return(element);
}

void
delete_clm_session(clm_session_t *session)
{
    saftest_list_element element;

    saftest_log("Deleting clm session with id %d\n", session->clm_session_id);

    element = lookup_clm_session_element(session->clm_session_id);
    saftest_list_element_delete(&element);
}

static clm_session_t *
lookup_clm_session(ubit32 clm_session_id)
{
    saftest_list_element element;
    
    element = lookup_clm_session_element(clm_session_id);
    return((clm_session_t *)element->data);
}

clm_session_t *
lookup_clm_session_by_invocation(SaInvocationT invocation)
{
    saftest_list_element element;
    clm_driver_thread_t *cdt;

    element = saftest_list_find(clm_global_session_list,
                                clm_session_invocation_comparator,
                                &invocation, NULL);
    if (NULL == element) {
        cdt = get_current_clm_thread_data();
        element = saftest_list_find(cdt->thread_local_session_list,
                                    clm_session_invocation_comparator,
                                    &invocation, NULL);
    }
    saftest_assert(NULL != element,
                   "Attempt to find unknown session");
    return((clm_session_t *)element->data);
}

clm_session_t *
lookup_clm_session_from_request(saftest_msg_t *request)
{
    clm_session_t *session;

    session = lookup_clm_session(
                  saftest_msg_get_ubit32_value(request, "CLM_SESSION_ID"));
    return(session);
}

clm_session_t *
lookup_long_lived_clm_session_with_track_callback()
{
    saftest_list_element element;
    clm_driver_thread_t *cdt = get_current_clm_thread_data();
    clm_session_t *session;

    for (element = saftest_list_first(cdt->thread_local_session_list);
         NULL != element;
         element = saftest_list_next(element)) {
        session = (clm_session_t *)element->data;
        if (session->long_lived &&
            NULL != session->clm_callbacks.saClmClusterTrackCallback) {
            return(session);
        }
    }
    saftest_abort("Unable to find a long lived clm_session "
                  "with track callback\n");
    return(NULL);
}

void
saftest_daemon_write_cluster_node(FILE *fp, 
                                  const SaClmClusterNodeT *cluster_node)
{
    char node_name[SA_MAX_NAME_LENGTH+1];
    const char *family;
    struct in_addr in_addr;
    char addr_buf[INET6_ADDRSTRLEN];

    memset(node_name, 0, sizeof(node_name));
    strncpy(node_name, 
            (char *) cluster_node->nodeName.value,
            cluster_node->nodeName.length);
    fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(fp, "<SAFNode "
                " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                " xsi:noNamespaceSchemaLocation=\"SAFNode.xsd\" "
                " schemaVersion=\"1\"> \n");

    fprintf(fp, "    <id>%ld</id>\n", cluster_node->nodeId);
    fprintf(fp, "    <AddressList>\n");
    fprintf(fp, "        <Address>\n");
    if (SA_CLM_AF_INET == cluster_node->nodeAddress.family) {
        family = "SA_CLM_AF_INET";
    } else if (SA_CLM_AF_INET6 == cluster_node->nodeAddress.family) {
        family = "SA_CLM_AF_INET6";
    } else {
        saftest_abort("Unknown address family\n");
    }
    fprintf(fp, "            <family>%s</family>\n", family);
    fprintf(fp, "            <length>%d</length>\n",
            cluster_node->nodeAddress.length);

    memcpy(&in_addr.s_addr, cluster_node->nodeAddress.value,
            cluster_node->nodeAddress.length);
    strcpy(addr_buf, inet_ntoa(in_addr));
    fprintf(fp, "            <value>%s</value>\n",
            addr_buf);
    fprintf(fp, "        </Address>\n");
    fprintf(fp, "    </AddressList>\n");
    fprintf(fp, "    <name>%s</name>\n", 
            node_name);
    fprintf(fp, "    <member>%s</member>\n", 
            cluster_node->member ?
            "TRUE" : "FALSE");
    fprintf(fp, "    <bootTimestamp>%lld</bootTimestamp>\n", 
            cluster_node->bootTimestamp);
    fprintf(fp, "    <initialViewNumber>%lld</initialViewNumber>\n",
            cluster_node->initialViewNumber);
    fprintf(fp, "</SAFNode>\n");
}

void
saftest_daemon_write_cluster(FILE *fp, 
                             const SaClmClusterNotificationBufferT *buf)
{
    char node_name[SA_MAX_NAME_LENGTH+1];
    const char *family;
    const char *changeFlag;
    SaUint32T ndx;
    struct in_addr in_addr;
    char addr_buf[INET6_ADDRSTRLEN];

    memset(node_name, 0, sizeof(node_name));

    fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(fp, "<SAFCluster "
                " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                " xsi:noNamespaceSchemaLocation=\"SAFCluster.xsd\" "
                " schemaVersion=\"1\"> \n");

    fprintf(fp, "    <SAFNodeList>\n");

    for (ndx = 0; ndx < buf->numberOfItems; ndx++) {
        memset(node_name, 0, sizeof(node_name));
        strncpy(node_name, 
                (char *) buf->notification[ndx].clusterNode.nodeName.value,
                sizeof(node_name)); 
                /* buf->notification[ndx].clusterNode.nodeName.length); */
        fprintf(fp, "        <SAFNode>\n");
        fprintf(fp, "            <id>%ld</id>\n", 
                buf->notification[ndx].clusterNode.nodeId);
        fprintf(fp, "            <AddressList>\n");
        fprintf(fp, "                <Address>\n");
        if (SA_CLM_AF_INET ==
            buf->notification[ndx].clusterNode.nodeAddress.family) {
            family = "SA_CLM_AF_INET";
        } else if (SA_CLM_AF_INET6 ==
                   buf->notification[ndx].clusterNode.nodeAddress.family) {
            family = "SA_CLM_AF_INET6";
        } else {
            saftest_abort("Unknown address family\n");
        }
        fprintf(fp, "                    <family>%s</family>\n", family);
        fprintf(fp, "                    <length>%d</length>\n",
                buf->notification[ndx].clusterNode.nodeAddress.length);

        memcpy(&in_addr.s_addr,
                buf->notification[ndx].clusterNode.nodeAddress.value, 
                buf->notification[ndx].clusterNode.nodeAddress.length);
        strcpy(addr_buf, inet_ntoa(in_addr));
        fprintf(fp, "                    <value>%s</value>\n",
                addr_buf);
        fprintf(fp, "                </Address>\n");
        fprintf(fp, "            </AddressList>\n");
        fprintf(fp, "            <name>%s</name>\n", 
                node_name);
        fprintf(fp, "            <member>%s</member>\n", 
                buf->notification[ndx].clusterNode.member ?
                "TRUE" : "FALSE");
        fprintf(fp, "            <bootTimestamp>%lld</bootTimestamp>\n", 
                buf->notification[ndx].clusterNode.bootTimestamp);
        fprintf(fp, "            <initialViewNumber>%lld</initialViewNumber>\n",
                buf->notification[ndx].clusterNode.initialViewNumber);
        fprintf(fp, "        </SAFNode>\n");
    }

    fprintf(fp, "    </SAFNodeList>\n");

    fprintf(fp, "    <SAFNodeChangeFlagList>\n");

    for (ndx = 0; ndx < buf->numberOfItems; ndx++) {
        fprintf(fp, "        <SAFNodeChangeFlag>\n");
        fprintf(fp, "            <id>%ld</id>\n", 
                buf->notification[ndx].clusterNode.nodeId);
        if (SA_CLM_NODE_NO_CHANGE ==
            buf->notification[ndx].clusterChange) {
            changeFlag = "SA_CLM_NODE_NO_CHANGE";
        } else if (SA_CLM_NODE_JOINED ==
            buf->notification[ndx].clusterChange) {
            changeFlag = "SA_CLM_NODE_JOINED";
        } else if (SA_CLM_NODE_LEFT ==
            buf->notification[ndx].clusterChange) {
            changeFlag = "SA_CLM_NODE_LEFT";
        } else if (SA_CLM_NODE_RECONFIGURED ==
            buf->notification[ndx].clusterChange) {
            changeFlag = "SA_CLM_NODE_RECONFIGURED";
        } else {
            saftest_abort("Unknown cluster change flag\n");
        }
        fprintf(fp, "            <changeFlag>%s</changeFlag>\n", changeFlag);
        fprintf(fp, "        </SAFNodeChangeFlag>\n");
    }

    fprintf(fp, "    </SAFNodeChangeFlagList>\n");
    fprintf(fp, "</SAFCluster>\n");
}

void
saftest_daemon_cluster_node_get_callback(SaInvocationT invocation,
                                         const SaClmClusterNodeT *cluster_node,
                                         SaAisErrorT error)
{
    clm_session_t *clm_session = NULL;
    FILE *fp = NULL;

    saftest_log("Cluster Node Get Callback for invocation %lld with "
                "status %d\n", invocation, error);
    clm_session = lookup_clm_session_by_invocation(invocation);
    if (NULL == clm_session) {
        saftest_abort("Unknown invocation id %lld\n", invocation);
    }
    if ((SA_AIS_OK == error) &&
        (strlen(clm_session->cluster_node_get_callback_xml_file) > 0)) {
        fp = fopen(clm_session->cluster_node_get_callback_xml_file, "w+");
        if (NULL == fp) {
            saftest_abort("Unable to open %s for writing\n",
                          clm_session->cluster_node_get_callback_xml_file);
        }
        saftest_daemon_write_cluster_node(fp, cluster_node);
        fclose(fp);
    }
    clm_session->cluster_node_get_callback_count += 1;
    saftest_log("cluster_node_get_callback_count for id %d is %d\n",
                clm_session->clm_session_id,
                clm_session->cluster_node_get_callback_count);
}

void
saftest_daemon_cluster_track_callback(
    const SaClmClusterNotificationBufferT *notificationBuffer,
    SaUint32T numberOfMembers,
    SaAisErrorT error)
{
    clm_session_t *clm_session = NULL;
    FILE *fp = NULL;

    /*
     * We only increment the cluster track callback count on the first
     * long lived session.  There is no incarnation number for this particular
     * callback so there is no way for the callback function to have any
     * sort of context.  As a result, the test cases and this driver have to
     * assume that when you call the asynchronous version of 
     * saClmClusterTrack(..TRACK_CURRENT..) that it is always on the first
     * long lived session.
     */
    clm_session = lookup_long_lived_clm_session_with_track_callback();
    clm_session->cluster_track_callback_count += 1;
    saftest_log("Received a cluster track callback with status %d.  "
                "Callback count is now %d on id %d\n", error, 
                clm_session->cluster_track_callback_count,
                clm_session->clm_session_id);

    if ((SA_AIS_OK == error) &&
        (strlen(clm_session->cluster_track_callback_xml_file) > 0)) {
        fp = fopen(clm_session->cluster_track_callback_xml_file, "w+");
        saftest_daemon_write_cluster(fp, notificationBuffer);
        fclose(fp);
    }
    /* !!! We need to de-allocate the notification buffer */
}

void
saftest_daemon_handle_create_session_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_session_t *clm_session;
    clm_driver_thread_t *cdt;
    
    saftest_log("Received a create test session request.\n");

    if (0 == strcmp(saftest_msg_get_str_value(request, 
                                              "CLM_SESSION_GLOBAL"), "TRUE")) {
        clm_session = add_clm_session(clm_global_session_list);
    } else {
        saftest_assert(0 == 
                       strcmp(saftest_msg_get_str_value(request,
                                                        "CLM_SESSION_GLOBAL"),
                              "FALSE"),
                       "CLM_SESSION_GLOBAL must be TRUE or FALSE");
        cdt = get_current_clm_thread_data();
        clm_session = add_clm_session(cdt->thread_local_session_list);
    }

    if (0 == strcmp(saftest_msg_get_str_value(request, 
                                              "CLM_SESSION_LONG_LIVED"),
                    "TRUE")) {
        clm_session->long_lived = 1;
    } else {
        saftest_assert(0 == 
                       strcmp(saftest_msg_get_str_value(request,
                                                        "CLM_SESSION_LONG_LIVED"),
                              "FALSE"),
                       "CLM_SESSION_LONG_LIVED must be TRUE or FALSE");
        clm_session->long_lived = 0;
    }

    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op,
                                        SA_AIS_OK);
    saftest_msg_set_ubit32_value((*reply), "CLM_SESSION_ID",
                                 clm_session->clm_session_id);
}

void
saftest_daemon_handle_delete_session_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_session_t *clm_session;
    
    saftest_log("Received a delete request from for id %d\n",
                 saftest_msg_get_ubit32_value(request, "CLM_SESSION_ID"));

    clm_session = lookup_clm_session_from_request(request);
    
    saftest_assert(0 == clm_session->clm_handle,
                   "Session must be finalized before deletion\n");

    delete_clm_session(clm_session);

    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op,
                                        SA_AIS_OK);
    saftest_msg_set_ubit32_value((*reply), "CLM_SESSION_ID",
                                 clm_session->clm_session_id);
}

static void
clm_add_sessions_to_reply(
    saftest_list session_list,
    ubit32 *num_sessions,
    saftest_msg_t **reply)
{
    clm_session_t *session;
    ubit32 ndx;
    saftest_list_element element;
    char key[SAFTEST_STRING_LENGTH+1];

    for (ndx = *num_sessions, 
         element = saftest_list_first(session_list);
         NULL != element;
         ndx++, (*num_sessions)++, element = saftest_list_next(element)) {
        session = (clm_session_t *)element->data;
        sprintf(key, "CLM_SESSION_%d_ID", ndx);
        saftest_msg_set_ubit32_value((*reply), key, session->clm_session_id);
        sprintf(key, "CLM_SESSION_%d_DISPATCH_FLAGS", ndx);
        saftest_msg_set_str_value((*reply), key,
                                  saftest_dispatch_flags_to_string(
                                      session->dispatch_flags));
        sprintf(key, "CLM_SESSION_%d_LONG_LIVED", ndx);
        if (session->long_lived) {
            saftest_msg_set_str_value((*reply), key, "TRUE");
        } else {
            saftest_msg_set_str_value((*reply), key, "FALSE");
        }

        sprintf(key, "CLM_SESSION_%d_GLOBAL", ndx);
        if (session_list == clm_global_session_list) {
            saftest_msg_set_str_value((*reply), key, "TRUE");
        } else {
            saftest_msg_set_str_value((*reply), key, "FALSE");
        }
    }
}

void
saftest_daemon_handle_status_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    ubit32 num_sessions = 0;
    clm_driver_thread_t *cdt = get_current_clm_thread_data();

    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        SA_AIS_OK);
    clm_add_sessions_to_reply(clm_global_session_list, &num_sessions, reply);
    clm_add_sessions_to_reply(cdt->thread_local_session_list, &num_sessions, 
                              reply);
    saftest_msg_set_ubit32_value((*reply), "NUM_CLM_SESSIONS", num_sessions);
}

void
saftest_daemon_handle_init_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_session_t *clm_session;
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
                 saftest_msg_get_ubit32_value(request, "CLM_SESSION_ID"),
                 releaseCode,
                 saftest_msg_get_ubit32_value(request, "VERSION_MAJOR"),
                 saftest_msg_get_ubit32_value(request, "VERSION_MINOR"));

    clm_session = lookup_clm_session_from_request(request);
    clm_session->clm_callbacks.saClmClusterNodeGetCallback = NULL;
    clm_session->clm_callbacks.saClmClusterTrackCallback = NULL;

    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_CLM_HANDLE"))) {
        handle = &clm_session->clm_handle;
    }
    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_CALLBACKS"))) {
        callbacks = &clm_session->clm_callbacks;
        if (0 == strcmp("TRUE",
                        saftest_msg_get_str_value(request,
                                                  "CLUSTER_NODE_GET_CB"))) {
            clm_session->clm_callbacks.saClmClusterNodeGetCallback =
                saftest_daemon_cluster_node_get_callback;
        }
        if (0 == strcmp("TRUE",
                        saftest_msg_get_str_value(request,
                                                  "CLUSTER_TRACK_CB"))) {
            clm_session->clm_callbacks.saClmClusterTrackCallback =
                saftest_daemon_cluster_track_callback;
        }
    }
    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_VERSION"))) {
        version = &clm_session->version;
        clm_session->version.releaseCode = releaseCode;
        clm_session->version.majorVersion =
            saftest_msg_get_ubit8_value(request, "VERSION_MAJOR");
        clm_session->version.minorVersion =
            saftest_msg_get_ubit8_value(request, "VERSION_MINOR");
    }

    clm_session->dispatch_flags =
        saftest_dispatch_flags_from_string(
                   saftest_msg_get_str_value(request, "DISPATCH_FLAGS"));

    status = saClmInitialize(handle, callbacks, version);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_selection_object_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_session_t *clm_session = NULL;
    SaSelectionObjectT *selection_object = NULL;
    SaAisErrorT status;

    saftest_log("Received a select object get request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_SESSION_ID"));
    clm_session = lookup_clm_session_from_request(request);

    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request,
                                              "NULL_SELECTION_OBJECT"))) {
        selection_object = &clm_session->selection_object;
    }
    status = saClmSelectionObjectGet(clm_session->clm_handle, selection_object);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_dispatch_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_session_t *clm_session = NULL;
    SaAisErrorT status;
    SaDispatchFlagsT dispatch_flags;
    clm_driver_thread_t *cdt = get_current_clm_thread_data();

    saftest_log("Received a dispatch request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_SESSION_ID"));
    clm_session = lookup_clm_session_from_request(request);

    dispatch_flags =
        saftest_dispatch_flags_from_string(
                   saftest_msg_get_str_value(request, "DISPATCH_FLAGS"));
    saftest_assert(SAFTEST_DISPATCH_NONE != dispatch_flags,
                   "Can't use SA_DISPATCH_NONE for a dispatch request\n");
    saftest_assert((SA_DISPATCH_BLOCKING != dispatch_flags) ||
                   (FALSE == cdt->main_thread),
                   "You can't call DISPATCH_BLOCKING in the main thread\n");
    status = saClmDispatch(clm_session->clm_handle, dispatch_flags);
    if (SA_DISPATCH_BLOCKING != dispatch_flags) {
        /* 
         * The client is only expecting a reply in non dispatch blocking 
         * cases.  Otherwise the client would hang until we finalized.
         */
        (*reply) = saftest_reply_msg_create(request, 
                                            map_entry->reply_op, status);
    }
}

void
saftest_daemon_handle_session_finalize_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_session_t *clm_session = NULL;
    SaAisErrorT status;

    saftest_log("Received a finalize request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_SESSION_ID"));
    clm_session = lookup_clm_session_from_request(request);

    status = saClmFinalize(clm_session->clm_handle);
    clm_session->selection_object = 0;
    clm_session->clm_handle = 0;
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_cluster_node_get_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_session_t *clm_session = NULL;
    SaAisErrorT status;
    SaClmNodeIdT node_id;
    SaTimeT timeout;
    const char *xml_file = NULL;
    FILE *fp = NULL;
    SaClmClusterNodeT cluster_node;
    SaClmClusterNodeT *cluster_node_ptr = NULL;

    saftest_log("Received a cluster node get request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_SESSION_ID"));
    clm_session = lookup_clm_session_from_request(request);
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
    if (0 == strcmp(saftest_msg_get_str_value(request, "TIMEOUT"), 
                                              "SA_TIME_MAX")) {
        timeout = SA_TIME_MAX;
    } else {
        timeout = saftest_msg_get_sbit64_value(request, "TIMEOUT");
    }
    status = saClmClusterNodeGet(clm_session->clm_handle, 
                                 node_id, timeout, cluster_node_ptr);
    if ((SA_AIS_OK == status) &&
        (NULL != (xml_file = saftest_msg_get_str_value(request, "XML_FILE")))) {
        saftest_assert(NULL != cluster_node_ptr,
                       "Can't ask for an XML_FILE without a cluster_node");
        fp = fopen(xml_file, "w+");
        if (NULL == fp) {
            saftest_abort("Unable to open %s for writing\n", xml_file);
        }

        saftest_daemon_write_cluster_node(fp, cluster_node_ptr);
        fclose(fp);    
        (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                            SA_AIS_OK);
    }
    
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_cluster_node_get_async_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_session_t *clm_session = NULL;
    SaAisErrorT status;
    SaClmNodeIdT node_id;

    saftest_log("Received a cluster node get async request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_SESSION_ID"));
    clm_session = lookup_clm_session_from_request(request);

    if (saftest_msg_has_key(request, "XML_FILE")) {
        strcpy(clm_session->cluster_node_get_callback_xml_file, 
               saftest_msg_get_str_value(request, "XML_FILE"));
    } else {
        memset(clm_session->cluster_node_get_callback_xml_file, 0,
               sizeof(clm_session->cluster_node_get_callback_xml_file));
    }
    clm_session->cluster_node_get_async_invocation = 
                      saftest_msg_get_ubit64_value(request, "INVOCATION");
    node_id = get_node_id_from_string(
                      saftest_msg_get_str_value(request, "NODE_ID"));
    status = saClmClusterNodeGetAsync(
                      clm_session->clm_handle, 
                      saftest_msg_get_ubit64_value(request, "INVOCATION"),
                      node_id);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_cluster_node_get_reset_cb_count_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_session_t *clm_session = NULL;

    saftest_log("Received a cluster node get reset count request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_SESSION_ID"));
    clm_session = lookup_clm_session_from_request(request);

    clm_session->cluster_node_get_callback_count = 0;
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        SA_AIS_OK);
}

void
saftest_daemon_handle_cluster_node_get_cb_count_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_session_t *clm_session = NULL;

    saftest_log("Received a cluster node get cb count request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_SESSION_ID"));
    clm_session = lookup_clm_session_from_request(request);

    saftest_log("Cluster node get cb count for id %d is %d\n",
                clm_session->clm_session_id,
                clm_session->cluster_node_get_callback_count);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        SA_AIS_OK);
    saftest_msg_set_ubit32_value(*reply, 
                                 "NODE_GET_CALLBACK_COUNT",
                                 clm_session->cluster_node_get_callback_count);
}

void
saftest_daemon_handle_cluster_node_get_async_invocation_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_session_t *clm_session = NULL;

    saftest_log("Received a cluster node get async invocation request for id "
                 "%d\n",
                saftest_msg_get_ubit32_value(request, "CLM_SESSION_ID"));
    clm_session = lookup_clm_session_from_request(request);

    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        SA_AIS_OK);
    saftest_msg_set_ubit32_value(*reply, 
                                 "NODE_GET_ASYNC_INVOCATION",
                                 clm_session->cluster_node_get_async_invocation);
}

void
saftest_daemon_handle_cluster_track_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_session_t *clm_session = NULL;
    SaClmClusterNotificationBufferT *buffer = NULL;
    SaClmClusterNotificationBufferT real_buffer;
    SaAisErrorT status;
    SaUint8T track_flags = 0;
    const char *xml_file = NULL;
    FILE *fp = NULL;

    saftest_log("Received a cluster track request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_SESSION_ID"));
    clm_session = lookup_clm_session_from_request(request);

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
    if ((track_flags & SA_TRACK_CURRENT) &&
        (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request,
                                              "NULL_NOTIFICATION_BUFFER")))) {
        buffer = &real_buffer;
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
    clm_session->track_flags = track_flags;
    if (((track_flags & SA_TRACK_CHANGES) && 
        !(track_flags & SA_TRACK_CHANGES_ONLY)) ||
        ((track_flags & SA_TRACK_CHANGES_ONLY) && 
         !(track_flags & SA_TRACK_CHANGES))) {
        saftest_log("Will write the callback info to %s\n",
                    saftest_msg_get_str_value(request, "XML_FILE"));
        strcpy(clm_session->cluster_track_callback_xml_file, 
               saftest_msg_get_str_value(request, "XML_FILE"));
    } else {
        memset(clm_session->cluster_track_callback_xml_file, 0,
               sizeof(clm_session->cluster_track_callback_xml_file));
    }

    status = saClmClusterTrack(clm_session->clm_handle, clm_session->track_flags,
                               buffer);
    if ((SA_AIS_OK == status) &&
        (track_flags & SA_TRACK_CURRENT) && (NULL != buffer)) {
        xml_file = saftest_msg_get_str_value(request, "XML_FILE");
        saftest_assert(NULL != xml_file,
                       "Must provide an XML_FILE");
        fp = fopen(xml_file, "w+");
        if (NULL == fp) {
            saftest_abort("Unable to open %s for writing\n", xml_file);
        }
        saftest_daemon_write_cluster(fp, buffer);
        fclose(fp);
    }
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_cluster_track_stop_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_session_t *clm_session = NULL;
    SaAisErrorT status;

    saftest_log("Received a cluster track stop request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_SESSION_ID"));
    clm_session = lookup_clm_session_from_request(request);

    status = saClmClusterTrackStop(clm_session->clm_handle);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_cluster_track_reset_cb_count_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_session_t *clm_session = NULL;

    saftest_log("Received a cluster track reset count request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_SESSION_ID"));
    clm_session = lookup_clm_session_from_request(request);

    saftest_log("cluster track reset callback count for id %d\n",
                clm_session->clm_session_id);
    clm_session->cluster_track_callback_count = 0;
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        SA_AIS_OK);
}

void
saftest_daemon_handle_cluster_track_cb_count_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_session_t *clm_session = NULL;

    saftest_log("Received a cluster track count request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_SESSION_ID"));
    clm_session = lookup_clm_session_from_request(request);

    saftest_log("cluster track callback count for id %d is %d\n",
                clm_session->clm_session_id, 
                clm_session->cluster_track_callback_count);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        SA_AIS_OK);
    saftest_msg_set_ubit32_value(*reply, "CLUSTER_TRACK_CALLBACK_COUNT",
                                 clm_session->cluster_track_callback_count);
}

void 
saftest_daemon_handle_finalize_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    clm_session_t *clm_session = NULL;
    SaAisErrorT status;

    saftest_log("Received a finalize request for id %d\n",
                saftest_msg_get_ubit32_value(request, "CLM_SESSION_ID"));
    clm_session = lookup_clm_session_from_request(request);

    status = saClmFinalize(clm_session->clm_handle);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        status);
    clm_session->selection_object = 0;
}

void saftest_daemon_add_clm_session_to_fdset(void *data,
                                              void *key)
{
    clm_session_t *clm_session;
    fd_set_key_t *set_key = (fd_set_key_t *)key;

    if (NULL == data) {
        return;
    }

    clm_session = (clm_session_t *)data;
    if (SA_DISPATCH_BLOCKING == clm_session->dispatch_flags) {
        /* It will have its own dispatch thread */
        return;
    }

    if (clm_session->selection_object > 0) {
        FD_SET(clm_session->selection_object, set_key->set);
        if (clm_session->selection_object > set_key->largest_fd) {
            set_key->largest_fd = clm_session->selection_object;
        }
    }
    return;
}

void
saftest_daemon_handle_incoming_clm_message(void *data, void *key)
{
    clm_session_t *clm_session = data;
    fd_set *fd_mask = (fd_set *)key;
    SaAisErrorT err;

    if (NULL == data) {
        return;
    }

    if (!FD_ISSET(clm_session->selection_object, fd_mask)) {
        return;
    }

    saftest_log("Incoming request on clm selection fd %d.  "
                 "Calling saClmDispatch\n",
                 clm_session->selection_object);
    saftest_assert(SA_DISPATCH_BLOCKING != clm_session->dispatch_flags,
                   "It will have its own dispatch thread\n");
    err = saClmDispatch(clm_session->clm_handle, clm_session->dispatch_flags);
    if (SA_AIS_OK != err) {
        saftest_log("Error %s performing saClmDispatch\n",
                     get_error_string(err));
        /* !!! We may only want to do this for "short-lived" drivers */
        exit(1);
    }
}

void saftest_daemon_add_fds(
    int *max_fd,
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    fd_set_key_t set_key;
    clm_driver_thread_t *cdt = get_current_clm_thread_data();

    set_key.set = read_fd_set;
    set_key.largest_fd = *max_fd;

    saftest_list_each(cdt->thread_local_session_list, 
                      saftest_daemon_add_clm_session_to_fdset,
                      &set_key);
    *max_fd = set_key.largest_fd;
}

void
saftest_daemon_check_fds(
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    clm_driver_thread_t *cdt = get_current_clm_thread_data();

    saftest_list_each(cdt->thread_local_session_list,
                      saftest_daemon_handle_incoming_clm_message,
                      read_fd_set);
}

SaAisErrorT
saftest_client_handle_create_session_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply = NULL;
    SaAisErrorT status;
 
    saftest_send_request(fd, SAFTEST_MSG_DESTINATION_LIBRARY, 
                         get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");

    status = saftest_reply_msg_get_status(reply);
    if (SA_AIS_OK == status) {
        saftest_log("CLM_SESSION_ID=%d\n", 
                    saftest_msg_get_ubit32_value(reply, 
                                                 "CLM_SESSION_ID"));
    }
    status = saftest_reply_msg_get_status(reply);
    saftest_msg_free(&reply);

    return(status);
}

SaAisErrorT
saftest_client_handle_status_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply = NULL;
    SaAisErrorT status;
    ubit32 ndx;
    ubit32 clm_session_id;
    char key[SAFTEST_STRING_LENGTH+1];
 
    saftest_send_request(fd, SAFTEST_MSG_DESTINATION_LIBRARY, 
                         get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");

    status = saftest_reply_msg_get_status(reply);
    if (SA_AIS_OK == status) {
        saftest_log("NUM_CLM_SESSIONS=%d\n", 
                    saftest_msg_get_ubit32_value(reply, 
                                                 "NUM_CLM_SESSIONS"));
        for (ndx = 0; 
             ndx < saftest_msg_get_ubit32_value(reply, "NUM_CLM_SESSIONS");
             ndx++) {
            sprintf(key, "CLM_SESSION_%d_ID", ndx);
            clm_session_id = saftest_msg_get_ubit32_value(reply, key);
            saftest_log("CLM_SESSION_%d_ID=%d\n",
                        ndx, clm_session_id);
            sprintf(key, "CLM_SESSION_%d_DISPATCH_FLAGS", ndx);
            saftest_log("CLM_SESSION_%d_DISPATCH_FLAGS=%s\n",
                        ndx, saftest_msg_get_str_value(reply, key));
            sprintf(key, "CLM_SESSION_%d_LONG_LIVED", ndx);
            saftest_log("CLM_SESSION_%d_LONG_LIVED=%s\n",
                        ndx, saftest_msg_get_str_value(reply, key));
            sprintf(key, "CLM_SESSION_%d_GLOBAL", ndx);
            saftest_log("CLM_SESSION_%d_GLOBAL=%s\n",
                        ndx, saftest_msg_get_str_value(reply, key));
        }
    }
    status = saftest_reply_msg_get_status(reply);
    saftest_msg_free(&reply);

    return(status);
}

SaAisErrorT
saftest_client_handle_dispatch_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply = NULL;
    SaAisErrorT status;
    SaDispatchFlagsT dispatch_flags;

    dispatch_flags =
        saftest_dispatch_flags_from_string(
                   saftest_msg_get_str_value(request, "DISPATCH_FLAGS"));
    if (SA_DISPATCH_BLOCKING == dispatch_flags) {
        /* 
         * This thread is about to go into a blocking call, so we don't
         * want to wait for a reply otherwise the test case will hang.
         */
        saftest_send_request(fd, SAFTEST_MSG_DESTINATION_LIBRARY, 
                             get_library_id(), request, NULL);
        status = SA_AIS_OK;
    } else {
        saftest_send_request(fd, SAFTEST_MSG_DESTINATION_LIBRARY, 
                             get_library_id(), request, &reply);
        saftest_assert(NULL != reply, "Received no reply from the daemon\n");
        status = saftest_reply_msg_get_status(reply);
        saftest_msg_free(&reply);
    }

    return(status);
}

SaAisErrorT
saftest_client_handle_cluster_node_get_cb_count_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply = NULL;
    SaAisErrorT status;
 
    saftest_send_request(fd, SAFTEST_MSG_DESTINATION_LIBRARY, 
                         get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");

    status = saftest_reply_msg_get_status(reply);
    if (SA_AIS_OK == status) {
        saftest_log("CLUSTER_NODE_GET_CALLBACK_COUNT=%d\n",
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
 
    saftest_send_request(fd, SAFTEST_MSG_DESTINATION_LIBRARY, 
                         get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");

    status = saftest_reply_msg_get_status(reply);
    if (SA_AIS_OK == status) {
        saftest_log("CLUSTER_NODE_GET_ASYNC_INVOCATION=%d\n",
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
 
    saftest_send_request(fd, SAFTEST_MSG_DESTINATION_LIBRARY, 
                         get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");

    status = saftest_reply_msg_get_status(reply);
    if (SA_AIS_OK == status) {
        saftest_log("CLUSTER_TRACK_CALLBACK_COUNT=%d\n",
                    saftest_msg_get_ubit32_value(
                        reply, 
                        "CLUSTER_TRACK_CALLBACK_COUNT"));
    }

    free(reply);
    return(status);
}

SAFTEST_MAP_TABLE_BEGIN(CLM)
SAFTEST_MAP_TABLE_ENTRY(
    "CREATE_SESSION_REQ", "CREATE_SESSION_REPLY",
    saftest_client_handle_create_session_request,
    saftest_daemon_handle_create_session_request)

SAFTEST_MAP_TABLE_ENTRY(
    "DELETE_SESSION_REQ", "DELETE_SESSION_REPLY",
    saftest_client_generic_handle_request,
    saftest_daemon_handle_delete_session_request)

SAFTEST_MAP_TABLE_ENTRY(
    "STATUS_REQ", "STATUS_REPLY",
    saftest_client_handle_status_request,
    saftest_daemon_handle_status_request)

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
     saftest_client_handle_dispatch_request,
     saftest_daemon_handle_dispatch_request)

SAFTEST_MAP_TABLE_ENTRY(
    "FINALIZE_REQ", "FINALIZE_REPLY",
     saftest_client_generic_handle_request,
     saftest_daemon_handle_session_finalize_request)

SAFTEST_MAP_TABLE_ENTRY(
    "CLUSTER_NODE_GET_REQ", "CLUSTER_NODE_GET_REPLY",
     saftest_client_generic_handle_request,
     saftest_daemon_handle_cluster_node_get_request)

SAFTEST_MAP_TABLE_ENTRY(
    "CLUSTER_NODE_GET_ASYNC_REQ", "CLUSTER_NODE_GET_ASYNC_REPLY",
     saftest_client_generic_handle_request,
     saftest_daemon_handle_cluster_node_get_async_request)

SAFTEST_MAP_TABLE_ENTRY(
    "CLUSTER_NODE_GET_RESET_CALLBACK_COUNT_REQ",
    "CLUSTER_NODE_GET_RESET_CALLBACK_COUNT_REPLY",
     saftest_client_generic_handle_request,
     saftest_daemon_handle_cluster_node_get_reset_cb_count_request)

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
    "CLUSTER_TRACK_RESET_CALLBACK_COUNT_REQ",
    "CLUSTER_TRACK_RESET_CALLBACK_COUNT_REPLY",
     saftest_client_generic_handle_request,
     saftest_daemon_handle_cluster_track_reset_cb_count_request)

SAFTEST_MAP_TABLE_ENTRY(
    "CLUSTER_TRACK_CALLBACK_COUNT_REQ",
    "CLUSTER_TRACK_CALLBACK_COUNT_REPLY",
     saftest_client_handle_cluster_track_cb_count_request,
     saftest_daemon_handle_cluster_track_cb_count_request)

SAFTEST_MAP_TABLE_END(CLM)

const char *get_library_id()
{
    return "CLM";
}
