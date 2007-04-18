/**********************************************************************
 *
 *	I N C L U D E S
 **********************************************************************/

#include "saftest_driver_lib_utils.h"
#include "saftest_driver.h"
#include "saftest_log.h"
#include "saftest_comm.h"
#include "saLck.h"

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

typedef struct lck_session {
    int lck_session_id;

    SaVersionT version;
    SaNameT lock_name;
    SaTimeT timeout;
    SaDispatchFlagsT dispatch_flags;
    SaLckHandleT lck_handle;
    SaSelectionObjectT selection_object;
    SaUint8T track_flags;
    SaLckCallbacksT lck_callbacks;
    SaLckResourceHandleT res_handle;
    SaLckLockIdT lock_id;
    SaLckLockModeT lock_mode;
    SaLckResourceOpenFlagsT lock_open_flags;

    SaLckLockStatusT lock_status;
    SaInvocationT async_lock_invocation;
    SaAisErrorT async_lock_error_status;
    SaInvocationT async_unlock_invocation;
    SaAisErrorT async_unlock_error_status;
    SaInvocationT async_open_invocation;
    SaAisErrorT async_open_error_status;

    int long_lived;
    SaLckWaiterSignalT last_delivered_waiter_signal;
    ubit32 waiter_signal_notification_count;
} lck_session_t;

typedef struct lck_driver_thread {
    int main_thread;
    pthread_t thread_id;
    saftest_list thread_local_session_list;
} lck_driver_thread_t;

saftest_list lck_global_session_list = NULL;
saftest_list lck_thread_data = NULL;

const char *get_library_id();

static lck_driver_thread_t *
add_lck_thread_data()
{
    lck_driver_thread_t *ldt;

    ldt = malloc(sizeof(lck_driver_thread_t));
    assert(NULL != ldt);
    memset(ldt, 0, sizeof(lck_driver_thread_t));

    ldt->thread_id = pthread_self();
    ldt->thread_local_session_list = saftest_list_create();

    saftest_list_element_create(lck_thread_data, ldt);
    return(ldt);
}

static lck_driver_thread_t *
get_current_lck_thread_data()
{
    saftest_list_element element;
    lck_driver_thread_t *ldt;

    for (element = saftest_list_first(lck_thread_data);
         NULL != element;
         element = saftest_list_next(element)) {
        ldt = (lck_driver_thread_t *)element->data;
        if (ldt->thread_id == pthread_self()) {
            return(ldt);
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

    lck_global_session_list = saftest_list_create();
    lck_thread_data = saftest_list_create();
}

void
saftest_daemon_thread_init(int main_thread)
{
    lck_driver_thread_t *ldt;

    ldt = add_lck_thread_data();
    ldt->main_thread = main_thread;
}

static const char *
saftest_map_lock_status_to_string(SaLckLockStatusT op)
{
    if (SA_LCK_LOCK_GRANTED == op) {
        return "SA_LCK_LOCK_GRANTED";
    } else if (SA_LCK_LOCK_DEADLOCK == op) {
        return "SA_LCK_LOCK_DEADLOCK";
    } else if (SA_LCK_LOCK_NOT_QUEUED == op) {
        return "SA_LCK_LOCK_NOT_QUEUED";
    } else if (SA_LCK_LOCK_ORPHANED == op) {
        return "SA_LCK_LOCK_ORPHANED";
    } else if (SA_LCK_LOCK_NO_MORE == op) {
        return "SA_LCK_LOCK_NO_MORE";
    } else if (SA_LCK_LOCK_DUPLICATE_EX == op) {
        return "SA_LCK_LOCK_DUPLICATE_EX";
    } else if (0 == op) {
        /* 
         * Special case to handle an async status request when we haven't
         * received the async callback yet.
         */
        return "SA_LCK_LOCK_NO_STATUS_YET";
    }
    saftest_abort("Unknown op status %d\n", op);
    return "Invalid Lock Status";
}

int
get_next_lck_session_id()
{
    static int next_lck_session_id = 1;
    int ret_id;

    ret_id = next_lck_session_id;
    next_lck_session_id += 1;
    return(ret_id);
}

static int
lck_session_id_comparator(void *data, void *key)
{
    lck_session_t *session = (lck_session_t *)data;
    ubit32 id = (*((ubit32 *)key));

    return(session->lck_session_id == id);
}

static lck_session_t *
add_lck_session(saftest_list session_list)
{
    lck_session_t *session;

    session = malloc(sizeof(lck_session_t));
    assert(NULL != session);
    memset(session, 0, sizeof(lck_session_t));

    saftest_list_element_create(session_list, session);
    session->lck_session_id = get_next_lck_session_id();
    saftest_log("Added a lck session with id %d\n", session->lck_session_id);
    return(session);
}

static saftest_list_element
lookup_lck_session_element(ubit32 lck_session_id)
{
    saftest_list_element element;
    lck_driver_thread_t *ldt;

    element = saftest_list_find(lck_global_session_list,
                                lck_session_id_comparator,
                                &lck_session_id, NULL);
    if (NULL == element) {
        ldt = get_current_lck_thread_data();
        element = saftest_list_find(ldt->thread_local_session_list,
                                    lck_session_id_comparator,
                                    &lck_session_id, NULL);
    }
    saftest_assert(NULL != element,
                   "Attempt to find unknown session");
    return(element);
}

static void
delete_lck_session(lck_session_t *session)
{
    saftest_list_element element;

    saftest_log("Deleting lck session with id %d\n", session->lck_session_id);

    element = lookup_lck_session_element(session->lck_session_id);
    saftest_list_element_delete(&element);
}

static lck_session_t *
lookup_lck_session(int lck_session_id)
{
    saftest_list_element element;

    element = lookup_lck_session_element(lck_session_id);
    return((lck_session_t *)element->data);
}

lck_session_t *
lookup_lck_session_by_lock_id(SaLckLockIdT lock_id)
{
    saftest_list_element element;
    lck_session_t *session;
    lck_driver_thread_t *ldt = get_current_lck_thread_data();

    for (element = saftest_list_first(lck_global_session_list);
         NULL != element;
         element = saftest_list_next(element)) {
        session = (lck_session_t *)element->data;
        if (session->lock_id == lock_id) {
            return(session);
        }
    }

    for (element = saftest_list_first(ldt->thread_local_session_list);
         NULL != element;
         element = saftest_list_next(element)) {
        session = (lck_session_t *)element->data;
        if (session->lock_id == lock_id) {
            return(session);
        }
    }
    return(NULL);
}

lck_session_t *
lookup_lck_session_by_lock_invocation(SaInvocationT invocation)
{
    saftest_list_element element;
    lck_session_t *session;
    lck_driver_thread_t *ldt = get_current_lck_thread_data();

    for (element = saftest_list_first(lck_global_session_list);
         NULL != element;
         element = saftest_list_next(element)) {
        session = (lck_session_t *)element->data;
        if (session->async_lock_invocation == invocation) {
            return(session);
        }
    }

    for (element = saftest_list_first(ldt->thread_local_session_list);
         NULL != element;
         element = saftest_list_next(element)) {
        session = (lck_session_t *)element->data;
        if (session->async_lock_invocation == invocation) {
            return(session);
        }
    }
    return(NULL);
}

lck_session_t *
lookup_lck_session_by_open_invocation(SaInvocationT invocation)
{
    saftest_list_element element;
    lck_session_t *session;
    lck_driver_thread_t *ldt = get_current_lck_thread_data();

    for (element = saftest_list_first(lck_global_session_list);
         NULL != element;
         element = saftest_list_next(element)) {
        session = (lck_session_t *)element->data;
        if (session->async_open_invocation == invocation) {
            return(session);
        }
    }
    for (element = saftest_list_first(ldt->thread_local_session_list);
         NULL != element;
         element = saftest_list_next(element)) {
        session = (lck_session_t *)element->data;
        if (session->async_open_invocation == invocation) {
            return(session);
        }
    }
    return(NULL);
}


lck_session_t *
lookup_lck_session_by_unlock_invocation(SaInvocationT invocation)
{
    saftest_list_element element;
    lck_session_t *session;
    lck_driver_thread_t *ldt = get_current_lck_thread_data();

    for (element = saftest_list_first(lck_global_session_list);
         NULL != element;
         element = saftest_list_next(element)) {
        session = (lck_session_t *)element->data;
        if (session->async_unlock_invocation == invocation) {
            return(session);
        }
    }
    for (element = saftest_list_first(ldt->thread_local_session_list);
         NULL != element;
         element = saftest_list_next(element)) {
        session = (lck_session_t *)element->data;
        if (session->async_unlock_invocation == invocation) {
            return(session);
        }
    }
    return(NULL);
}

lck_session_t *
lookup_lck_session_from_request(saftest_msg_t *request)
{
    lck_session_t *session;

    session = lookup_lck_session(
                  saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"));
    if (NULL == session) {
        saftest_abort("Unknown session id %d\n",
                      saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"));
    }
    return(session);
}

void
saftest_daemon_lock_waiter_callback(SaLckWaiterSignalT waiterSignal,
                                     SaLckLockIdT lockId,
                                     SaLckLockModeT modeHeld,
                                     SaLckLockModeT modeRequested)
{
    lck_session_t *session;

    session = lookup_lck_session_by_lock_id(lockId);
    if (NULL == session) {
        saftest_abort("Unknown lock id %d\n", lockId);
    }
    session->last_delivered_waiter_signal = waiterSignal;
    session->waiter_signal_notification_count += 1;
    saftest_log("Received lock waiter callback with signal %lld. "
                 "Notification Count is now %d\n",
                 waiterSignal, session->waiter_signal_notification_count);
}

void
saftest_daemon_lock_grant_callback(SaInvocationT invocation,
                                   SaLckLockStatusT lockStatus,
                                   SaAisErrorT error)
{
    lck_session_t *session;

    session = lookup_lck_session_by_lock_invocation(invocation);
    if (NULL == session) {
        saftest_abort("Unknown async lock invocation %d\n", invocation);
    }
    saftest_log("Received lock grant notification for invocation %lld with "
                "error code %s\n", invocation, get_error_string(error));
    session->lock_status = lockStatus;
    session->async_lock_error_status = error;
}

void
saftest_daemon_lck_session_open_callback(SaInvocationT invocation,
                                           SaLckResourceHandleT res_handle,
                                           SaAisErrorT error)
{

    lck_session_t *session;
 
    session = lookup_lck_session_by_open_invocation(invocation);

    if (NULL == session) {
        saftest_abort("Unknown async lock invocation %d\n", invocation);
    }
    
    saftest_log("Received resource open async notification for invocation %lld with "
                "error code %s, resource handle %llu\n", invocation, get_error_string(error)), res_handle;
    
    session->res_handle = res_handle;
    session->async_open_error_status = error;
}

void
saftest_daemon_lock_release_callback(SaInvocationT invocation,
                                     SaAisErrorT error)
{
    lck_session_t *session;

    session = lookup_lck_session_by_unlock_invocation(invocation);
    if (NULL == session) {
        saftest_abort("Unknown async unlock invocation %d\n", invocation);
    }
    saftest_log("Received lock release notification for invocation %lld with "
                "error code %s\n", invocation, get_error_string(error));
    session->lock_status = SA_LCK_LOCK_NO_MORE;
    session->async_unlock_error_status = error;
    session->lock_id = 0;
    session->last_delivered_waiter_signal = 0;
    session->waiter_signal_notification_count = 0;
}

void
saftest_daemon_handle_create_session_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lck_session_t *session;
    lck_driver_thread_t *ldt;

    saftest_log("Received a create test session request.\n");

    if (0 == strcmp(saftest_msg_get_str_value(request,
                                              "LCK_SESSION_GLOBAL"), "TRUE")) {
        session = add_lck_session(lck_global_session_list);
    } else {
        saftest_assert(0 ==
                       strcmp(saftest_msg_get_str_value(request,
                                                        "LCK_SESSION_GLOBAL"),
                              "FALSE"),
                       "LCK_SESSION_GLOBAL must be TRUE or FALSE");
        ldt = get_current_lck_thread_data();
        session = add_lck_session(ldt->thread_local_session_list);
    }

    if (0 == strcmp(saftest_msg_get_str_value(request,
                                              "LCK_SESSION_LONG_LIVED"),
                    "TRUE")) {
        session->long_lived = 1;
    } else {
        saftest_assert(0 ==
                       strcmp(saftest_msg_get_str_value(request,
                                                        "LCK_SESSION_LONG_LIVED"),
                              "FALSE"),
                       "LCK_SESSION_LONG_LIVED must be TRUE or FALSE");
        session->long_lived = 0;
    }

    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op,
                                        SA_AIS_OK);
    saftest_msg_set_ubit32_value((*reply), "LCK_SESSION_ID",
                                 session->lck_session_id);

}

void
saftest_daemon_handle_delete_session_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lck_session_t *session;

    saftest_log("Received a delete request from for id %d\n",
                 saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"));

    session = lookup_lck_session_from_request(request);

    saftest_assert(0 == session->lck_handle,
                   "Resource must be finalized before deletion\n");

    delete_lck_session(session);

    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op,
                                        SA_AIS_OK);
    saftest_msg_set_ubit32_value((*reply), "LCK_SESSION_ID",
                                 session->lck_session_id);
}

static void
lck_add_sessions_to_reply(
    saftest_list session_list,
    ubit32 *num_sessions,
    saftest_msg_t **reply)
{
    lck_session_t *session;
    ubit32 ndx;
    saftest_list_element element;
    char key[SAFTEST_STRING_LENGTH+1];

    for (ndx = *num_sessions,
         element = saftest_list_first(session_list);
         NULL != element;
         ndx++, (*num_sessions)++, element = saftest_list_next(element)) {
        session = (lck_session_t *)element->data;
        sprintf(key, "LCK_SESSION_%d_ID", ndx);
        saftest_msg_set_ubit32_value((*reply), key, session->lck_session_id);
        sprintf(key, "LCK_SESSION_%d_DISPATCH_FLAGS", ndx);
        saftest_msg_set_str_value((*reply), key,
                                  saftest_dispatch_flags_to_string(
                                      session->dispatch_flags));
        sprintf(key, "LCK_SESSION_%d_LONG_LIVED", ndx);
        if (session->long_lived) {
            saftest_msg_set_str_value((*reply), key, "TRUE");
        } else {
            saftest_msg_set_str_value((*reply), key, "FALSE");
        }

        sprintf(key, "LCK_SESSION_%d_GLOBAL", ndx);
        if (session_list == lck_global_session_list) {
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
    lck_driver_thread_t *ldt = get_current_lck_thread_data();

    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op,
                                        SA_AIS_OK);
    lck_add_sessions_to_reply(lck_global_session_list, &num_sessions, reply);
    lck_add_sessions_to_reply(ldt->thread_local_session_list, &num_sessions,
                              reply);
    saftest_msg_set_ubit32_value((*reply), "NUM_LCK_SESSIONS", num_sessions);
}

void
saftest_daemon_handle_init_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lck_session_t *session;
    SaLckHandleT *handle = NULL;
    SaLckCallbacksT *callbacks = NULL;
    SaVersionT *version = NULL;
    SaAisErrorT status;
    char *releaseCodeStr;
    char releaseCode;

    releaseCodeStr = saftest_msg_get_str_value(request,
                                               "VERSION_RELEASE_CODE");
    releaseCode = releaseCodeStr[0];
    saftest_log("Received an init request from for id %d "
                 "release code=%c, majorVersion=%d, minorVersion=%d\n",
                 saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"),
                 releaseCode,
                 saftest_msg_get_ubit32_value(request, "VERSION_MAJOR"),
                 saftest_msg_get_ubit32_value(request, "VERSION_MINOR"));

    session = lookup_lck_session_from_request(request);
    session->lck_callbacks.saLckResourceOpenCallback = NULL;
    session->lck_callbacks.saLckLockWaiterCallback = NULL;
    session->lck_callbacks.saLckLockGrantCallback = NULL;
    session->lck_callbacks.saLckResourceUnlockCallback = NULL;

    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_LCK_HANDLE"))) {
        handle = &session->lck_handle;
    }
    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_CALLBACKS"))) {
        callbacks = &session->lck_callbacks;
        if (0 == strcmp("TRUE",
                        saftest_msg_get_str_value(request,
                                                  "LOCK_WAITER_CB"))) {
            session->lck_callbacks.saLckLockWaiterCallback =
                saftest_daemon_lock_waiter_callback;
        }
        if (0 == strcmp("TRUE",
                        saftest_msg_get_str_value(request,
                                                  "RESOURCE_UNLOCK_CB"))) {
            session->lck_callbacks.saLckResourceUnlockCallback = 
                saftest_daemon_lock_release_callback;
        }
        if (0 == strcmp("TRUE",
                        saftest_msg_get_str_value(request,
                                                  "LOCK_GRANT_CB"))) {
            session->lck_callbacks.saLckLockGrantCallback = 
                saftest_daemon_lock_grant_callback;
        }
        
        if (0 == strcmp("TRUE",
                        saftest_msg_get_str_value(request,
                                                  "RESOURCE_OPEN_CB"))) {
            session->lck_callbacks.saLckResourceOpenCallback = 
                        saftest_daemon_lck_session_open_callback;
        }
    }
    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_VERSION"))) {
        version = &session->version;
        session->version.releaseCode = releaseCode;
        session->version.majorVersion =
            saftest_msg_get_ubit8_value(request, "VERSION_MAJOR");
        session->version.minorVersion =
            saftest_msg_get_ubit8_value(request, "VERSION_MINOR");
    }

    session->dispatch_flags =
        saftest_dispatch_flags_from_string(
                   saftest_msg_get_str_value(request, "DISPATCH_FLAGS"));

    status = saLckInitialize(handle, callbacks, version);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_selection_object_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lck_session_t *session = NULL;
    SaSelectionObjectT *selection_object = NULL;
    SaAisErrorT status;

    saftest_log("Received a select object get request for id %d\n",
                saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"));
    session = lookup_lck_session_from_request(request);

    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request,
                                              "NULL_SELECTION_OBJECT"))) {
        selection_object = &session->selection_object;
    }
    status = saLckSelectionObjectGet(session->lck_handle, selection_object);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_dispatch_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lck_session_t *session = NULL;
    SaAisErrorT status;
    SaDispatchFlagsT dispatch_flags;
    lck_driver_thread_t *ldt = get_current_lck_thread_data();

    saftest_log("Received a dispatch request for id %d\n",
                saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"));
    session = lookup_lck_session_from_request(request);

    dispatch_flags =
        saftest_dispatch_flags_from_string(
                   saftest_msg_get_str_value(request, "DISPATCH_FLAGS"));
    saftest_assert(SAFTEST_DISPATCH_NONE != dispatch_flags,
                   "Can't use SA_DISPATCH_NONE for a dispatch request\n");
    saftest_assert((SA_DISPATCH_BLOCKING != dispatch_flags) ||
                   (FALSE == ldt->main_thread),
                   "You can't call DISPATCH_BLOCKING in the main thread\n");
    status = saLckDispatch(session->lck_handle, dispatch_flags);
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
    lck_session_t *session = NULL;
    SaAisErrorT status;

    saftest_log("Received a finalize request for id %d\n",
                saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"));
    session = lookup_lck_session_from_request(request);

    status = saLckFinalize(session->lck_handle);
    session->lck_handle = 0;
    session->selection_object = 0;
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_lck_resource_open_async_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{

    lck_session_t *session = NULL;
    SaAisErrorT status;
    SaInvocationT invocation = 0;
    

    saftest_log("Received a resource open async request for id %d name %s\n",
                saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"),
                saftest_msg_get_str_value(request, "LOCK_NAME"));

    invocation = saftest_msg_get_ubit64_value(request, "INVOCATION");
    session = lookup_lck_session_from_request(request);
    session->lock_name.length =
       strlen(saftest_msg_get_str_value(request, "LOCK_NAME")) + 1;
    strcpy(((char *)session->lock_name.value),
       saftest_msg_get_str_value(request, "LOCK_NAME"));
    session->lock_open_flags |= SA_LCK_RESOURCE_CREATE;
    session->async_open_invocation = invocation;
    session->async_open_error_status = 0;

    status = saLckResourceOpenAsync(session->lck_handle,
                                    invocation,
                                    &session->lock_name,
                                    session->lock_open_flags);

    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
    
}
void 
saftest_daemon_handle_lck_session_open_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lck_session_t *session = NULL;
    SaAisErrorT status;

    saftest_log("Received a resource open request for id %d name %s\n",
                saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"),
                saftest_msg_get_str_value(request, "LOCK_NAME"));
    
    session = lookup_lck_session_from_request(request);

    session->lock_open_flags |= SA_LCK_RESOURCE_CREATE;
    session->lock_name.length = 
        strlen(saftest_msg_get_str_value(request, "LOCK_NAME")) + 1;
    strcpy(((char *)session->lock_name.value),
           saftest_msg_get_str_value(request, "LOCK_NAME"));
    status = saLckResourceOpen(session->lck_handle, 
                               &session->lock_name, 
                               session->lock_open_flags,
                               0, &session->res_handle);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_lck_resource_close_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lck_session_t *session = NULL;
    SaAisErrorT status;

    saftest_log("Received a resource close request for id %d\n",
                saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"));
    session = lookup_lck_session_from_request(request);
    status = saLckResourceClose(session->res_handle);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_sync_lock_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lck_session_t *session = NULL;
    SaAisErrorT status;
    SaTimeT timeout = 0;
    SaLckLockIdT *lock_id = NULL;
    SaLckLockStatusT *lock_status = NULL;
    SaLckLockFlagsT lock_flags = 0;
    SaLckLockModeT lock_mode = 0;
    SaLckWaiterSignalT waiter_signal = 0;

    if (0 == strcmp("TRUE",
                    saftest_msg_get_str_value(request, "LOCK_FLAG_NO_QUEUE"))) {
        lock_flags |= SA_LCK_LOCK_NO_QUEUE;
    }
    if (0 == strcmp("TRUE",
                    saftest_msg_get_str_value(request, "LOCK_FLAG_ORPHAN"))) {
        lock_flags |= SA_LCK_LOCK_ORPHAN;
    }
    if (0 == strcmp("TRUE",
                    saftest_msg_get_str_value(request, "LOCK_FLAG_INVALID"))) {
        lock_flags = -1;
    }

    if (0 == strcmp(saftest_msg_get_str_value(request, "LOCK_MODE"), "PR")) {
        lock_mode = SA_LCK_PR_LOCK_MODE;
    } else if (0 == strcmp(saftest_msg_get_str_value(request, 
                                                     "LOCK_MODE"), "EX")) {
        lock_mode = SA_LCK_EX_LOCK_MODE;
    } else if (0 == strcmp(saftest_msg_get_str_value(request, 
                                                     "LOCK_MODE"), "INVALID")) {
        lock_mode = -1;
    } else {
        saftest_abort("Unknown lock mode %s\n",
                      saftest_msg_get_str_value(request, "LOCK_MODE"));
    }

    waiter_signal = saftest_msg_get_ubit64_value(request, "WAITER_SIGNAL");
    
    timeout = saftest_time_from_string(saftest_msg_get_str_value(request, 
                                                                 "TIMEOUT"));

    saftest_log("Received a sync lock request with flags %x lock_mode %d "
                 "null_lock_id %s waiter_signal %lld for id %d\n",
                 lock_flags, lock_mode,
                 saftest_msg_get_str_value(request, "NULL_LOCK_ID"),
                 waiter_signal,
                 saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"));
    session = lookup_lck_session_from_request(request);

    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_LOCK_ID"))) {
        lock_id = &session->lock_id;
    }
    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_LOCK_STATUS"))) {
        lock_status = &session->lock_status;
    }
    status = saLckResourceLock(session->res_handle,
                               lock_id,
                               lock_mode,
                               lock_flags,
                               waiter_signal, timeout,
                               lock_status);
    saftest_log("saLckResourceLock returned %d, lock_status is %d\n",
                 status, session->lock_status);
    if (status == SA_AIS_OK) {
        if (0 ==

            strcmp(saftest_map_lock_status_to_string(session->lock_status), 
                   saftest_msg_get_str_value(request, 
                                             "EXPECTED_LOCK_STATUS"))) {
            status = SA_AIS_OK;
        } else {
            saftest_log("Expected lock status %s but got status %s\n",
                        saftest_msg_get_str_value(request, 
                                                  "EXPECTED_LOCK_STATUS"),
                        saftest_map_lock_status_to_string(
                            session->lock_status));
            status = SA_AIS_ERR_LIBRARY;
        }
    }
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_async_lock_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lck_session_t *session = NULL;
    SaAisErrorT status;
    SaLckLockIdT *lock_id = NULL;
    SaLckLockFlagsT lock_flags = 0;
    SaLckLockModeT lock_mode = 0;
    SaLckWaiterSignalT waiter_signal = 0;
    SaInvocationT invocation = 0;

    if (0 == strcmp("TRUE",
                    saftest_msg_get_str_value(request, "LOCK_FLAG_NO_QUEUE"))) {
        lock_flags |= SA_LCK_LOCK_NO_QUEUE;
    }
    if (0 == strcmp("TRUE",
                    saftest_msg_get_str_value(request, "LOCK_FLAG_ORPHAN"))) {
        lock_flags |= SA_LCK_LOCK_ORPHAN;
    }
    if (0 == strcmp("TRUE",
                    saftest_msg_get_str_value(request, "LOCK_FLAG_INVALID"))) {
        lock_flags = -1;
    }

    if (0 == strcmp(saftest_msg_get_str_value(request, "LOCK_MODE"), "PR")) {
        lock_mode = SA_LCK_PR_LOCK_MODE;
    } else if (0 == strcmp(saftest_msg_get_str_value(request, 
                                                     "LOCK_MODE"), "EX")) {
        lock_mode = SA_LCK_EX_LOCK_MODE;
    } else if (0 == strcmp(saftest_msg_get_str_value(request, 
                                                     "LOCK_MODE"), "INVALID")) {
        lock_mode = -1;
    } else {
        saftest_abort("Unknown lock mode %s\n",
                      saftest_msg_get_str_value(request, "LOCK_MODE"));
    }

    waiter_signal = saftest_msg_get_ubit64_value(request, "WAITER_SIGNAL");
    invocation = saftest_msg_get_ubit64_value(request, "INVOCATION");

    saftest_log("Received an async lock request with flags %x lock_mode %d "
                 "invocation %lld null_lock_id %s for id %d\n",
                 lock_flags, lock_mode, invocation,
                 saftest_msg_get_str_value(request, "NULL_LOCK_ID"),
                 saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"));
    session = lookup_lck_session_from_request(request);
    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_LOCK_ID"))) {
        lock_id = &session->lock_id;
    }
    session->async_lock_invocation = invocation;
    session->lock_status = 0;
    status = saLckResourceLockAsync(session->res_handle, invocation, lock_id, 
                                    lock_mode, lock_flags, waiter_signal);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_sync_unlock_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lck_session_t *session = NULL;
    SaTimeT timeout = 0;
    SaAisErrorT status;

    saftest_log("Received a blocking unlock request for id %d\n",
                 saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"));
    session = lookup_lck_session_from_request(request);
    status = saLckResourceUnlock(session->lock_id, timeout);
    if (SA_AIS_OK == status) {
        session->lock_id = 0;
        session->last_delivered_waiter_signal = 0;
        session->waiter_signal_notification_count = 0;
        session->lock_status = SA_LCK_LOCK_NO_MORE;
    }
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_async_unlock_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lck_session_t *session = NULL;
    SaInvocationT invocation = 0;
    SaAisErrorT status;

    invocation = saftest_msg_get_ubit64_value(request, "INVOCATION");
    saftest_log("Received an async unlock request for id %d invocation %lld\n",
                saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"),
                invocation);
    session = lookup_lck_session_from_request(request);
    session->async_unlock_invocation = invocation;
    status = saLckResourceUnlockAsync(invocation, session->lock_id);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_lock_get_wait_count_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lck_session_t *session = NULL;

    saftest_log("Received a signal notify count request for id %d\n",
                saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"));
    session = lookup_lck_session_from_request(request);

    saftest_assert(NULL != session->lck_callbacks.saLckLockWaiterCallback,
                   "Cant get the count on a session with no callback");
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        SA_AIS_OK);
    saftest_msg_set_ubit64_value(*reply, "LAST_DELIVERED_WAITER_SIGNAL",
                                 session->last_delivered_waiter_signal);
    saftest_msg_set_ubit32_value(*reply, "WAITER_SIGNAL_NOTIFICATION_COUNT",
                                 session->waiter_signal_notification_count);
}

void
saftest_daemon_handle_lock_get_resource_open_async_status_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{

    lck_session_t *session = NULL;

    saftest_log("Received an async open status request for id %d\n",
                saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"));
    session = lookup_lck_session_from_request(request);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        SA_AIS_OK);

    saftest_msg_set_ubit64_value(*reply, "ASYNC_OPEN_INVOCATION",
                                 session->async_open_invocation);
    saftest_msg_set_ubit32_value(*reply, "ASYNC_OPEN_ERROR_STATUS",
                                 session->async_open_error_status);
}

void 
saftest_daemon_handle_lock_get_async_lock_status_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lck_session_t *session = NULL;

    saftest_log("Received an async lock status request for id %d\n",
                saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"));
    session = lookup_lck_session_from_request(request);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        SA_AIS_OK);
    saftest_msg_set_ubit64_value(*reply, "LAST_DELIVERED_WAITER_SIGNAL",
                                 session->last_delivered_waiter_signal);
    saftest_msg_set_str_value(
        *reply, "LOCK_STATUS",
        saftest_map_lock_status_to_string(session->lock_status));
    
    saftest_msg_set_ubit64_value(*reply, "ASYNC_LOCK_INVOCATION",
                                 session->async_lock_invocation);
    saftest_msg_set_ubit32_value(*reply, "ASYNC_LOCK_ERROR_STATUS",
                                 session->async_lock_error_status);
}

void 
saftest_daemon_handle_lock_get_async_unlock_status_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lck_session_t *session = NULL;

    saftest_log("Received an async unlock status request for id %d\n",
                saftest_msg_get_ubit32_value(request, "LCK_SESSION_ID"));
    session = lookup_lck_session_from_request(request);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        SA_AIS_OK);
    saftest_msg_set_str_value(
        *reply, "LOCK_STATUS",
        saftest_map_lock_status_to_string(session->lock_status));
    saftest_msg_set_ubit64_value(*reply, "ASYNC_UNLOCK_INVOCATION",
                                 session->async_unlock_invocation);
    saftest_msg_set_ubit32_value(*reply, "ASYNC_UNLOCK_ERROR_STATUS",
                                 session->async_unlock_error_status);
}

void
saftest_daemon_handle_incoming_lck_message(void *data, void *key)
{
    lck_session_t *session = data;
    fd_set *fd_mask = (fd_set *)key;
    SaAisErrorT err;

    if (NULL == data) {
        return;
    }

    if (!FD_ISSET(session->selection_object, fd_mask)) {
        return;
    }

    saftest_log("Incoming request on lock selection fd %d\n",
                 session->selection_object);
    saftest_assert(SA_DISPATCH_BLOCKING != session->dispatch_flags,
                   "It will have its own dispatch thread\n");
    err = saLckDispatch(session->lck_handle, session->dispatch_flags);
    if (SA_AIS_OK != err) {
        saftest_log("Error %s performing saLckDispatch\n",
                     get_error_string(err));
    }
}

void saftest_daemon_add_lck_session_to_fdset(void *data, 
                                                void *key)
{
    lck_session_t *session;
    fd_set_key_t *set_key = (fd_set_key_t *)key;

    if (NULL == data) {
        return;
    }

    session = (lck_session_t *)data;
    if (SA_DISPATCH_BLOCKING == session->dispatch_flags) {
        /* It will have its own dispatch thread */
        return;
    }

    if (session->selection_object > 0) {
        FD_SET(session->selection_object, set_key->set);
        if (session->selection_object > set_key->largest_fd) {
            set_key->largest_fd = session->selection_object;
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
    lck_driver_thread_t *ldt = get_current_lck_thread_data();

    set_key.set = read_fd_set;
    set_key.largest_fd = *max_fd;

    saftest_list_each(ldt->thread_local_session_list,
                      saftest_daemon_add_lck_session_to_fdset,
                      &set_key);

    *max_fd = set_key.largest_fd;
}

void
saftest_daemon_check_fds(
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    lck_driver_thread_t *ldt = get_current_lck_thread_data();

    saftest_list_each(ldt->thread_local_session_list,
                      saftest_daemon_handle_incoming_lck_message,
                      read_fd_set);
}

SaAisErrorT
saftest_client_handle_create_session_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply;
    SaAisErrorT status;
 
    saftest_send_request(fd, SAFTEST_MSG_DESTINATION_LIBRARY,
                         get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");
    status = saftest_reply_msg_get_status(reply);

    if (SA_AIS_OK == status) {
        saftest_log("LCK_SESSION_ID=%d\n",
                    saftest_msg_get_ubit32_value(reply,
                                                 "LCK_SESSION_ID"));
    }
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
    ubit32 lck_session_id;
    char key[SAFTEST_STRING_LENGTH+1];

    saftest_send_request(fd, SAFTEST_MSG_DESTINATION_LIBRARY,
                         get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");

    status = saftest_reply_msg_get_status(reply);
    if (SA_AIS_OK == status) {
        saftest_log("NUM_LCK_SESSIONS=%d\n",
                    saftest_msg_get_ubit32_value(reply,
                                                 "NUM_LCK_SESSIONS"));
        for (ndx = 0;
             ndx < saftest_msg_get_ubit32_value(reply, "NUM_LCK_SESSIONS");
             ndx++) {
            sprintf(key, "LCK_SESSION_%d_ID", ndx);
            lck_session_id = saftest_msg_get_ubit32_value(reply, key);
            saftest_log("LCK_SESSION_%d_ID=%d\n",
                        ndx, lck_session_id);
            sprintf(key, "LCK_SESSION_%d_DISPATCH_FLAGS", ndx);
            saftest_log("LCK_SESSION_%d_DISPATCH_FLAGS=%s\n",
                        ndx, saftest_msg_get_str_value(reply, key));
            sprintf(key, "LCK_SESSION_%d_LONG_LIVED", ndx);
            saftest_log("LCK_SESSION_%d_LONG_LIVED=%s\n",
                        ndx, saftest_msg_get_str_value(reply, key));
            sprintf(key, "LCK_SESSION_%d_GLOBAL", ndx);
            saftest_log("LCK_SESSION_%d_GLOBAL=%s\n",
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
saftest_client_handle_lock_get_wait_count_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply;
    SaAisErrorT status;
 
    saftest_send_request(fd, SAFTEST_MSG_DESTINATION_LIBRARY,
                         get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");
    status = saftest_reply_msg_get_status(reply);

    if (SA_AIS_OK == status) {
        saftest_log("LAST_DELIVERED_WAITER_SIGNAL=%lld\n", 
                    saftest_msg_get_ubit64_value(
                        reply, "LAST_DELIVERED_WAITER_SIGNAL"));
        saftest_log("WAITER_SIGNAL_NOTIFICATION_COUNT=%d\n", 
                    saftest_msg_get_ubit32_value(
                        reply, "WAITER_SIGNAL_NOTIFICATION_COUNT"));
    }
    saftest_msg_free(&reply);
    return(status);
}

SaAisErrorT
saftest_client_handle_lock_get_resource_open_async_status_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply;
    SaAisErrorT status;
 
    saftest_send_request(fd, SAFTEST_MSG_DESTINATION_LIBRARY,
                         get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");
    status = saftest_reply_msg_get_status(reply);

    saftest_log("ASYNC_OPEN_INVOCATION=%d\n", 
                saftest_msg_get_ubit64_value(
                    reply, "ASYNC_OPEN_INVOCATION"));
    saftest_log("OPEN_STATUS=%s\n", 
                saftest_msg_get_str_value(reply, "OPEN_STATUS"));
    saftest_log("ASYNC_OPEN_ERROR_STATUS=%d\n", 
                saftest_msg_get_ubit32_value(reply, "ASYNC_OPEN_ERROR_STATUS"));
    saftest_msg_free(&reply);
    return(status);
}

SaAisErrorT
saftest_client_handle_lock_get_async_lock_status_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply;
    SaAisErrorT status;
 
    saftest_send_request(fd, SAFTEST_MSG_DESTINATION_LIBRARY,
                         get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");
    status = saftest_reply_msg_get_status(reply);

    saftest_log("ASYNC_LOCK_INVOCATION=%d\n", 
                saftest_msg_get_ubit64_value(
                    reply, "ASYNC_LOCK_INVOCATION"));
    saftest_log("LOCK_STATUS=%s\n", 
                saftest_msg_get_str_value(reply, "LOCK_STATUS"));
    saftest_log("ASYNC_LOCK_ERROR_STATUS=%d\n", 
                saftest_msg_get_ubit32_value(reply, "ASYNC_LOCK_ERROR_STATUS"));
    saftest_msg_free(&reply);
    return(status);
}

SaAisErrorT
saftest_client_handle_lock_get_async_unlock_status_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply;
    SaAisErrorT status;
 
    saftest_send_request(fd, SAFTEST_MSG_DESTINATION_LIBRARY,
                         get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");
    status = saftest_reply_msg_get_status(reply);

    saftest_log("ASYNC_UNLOCK_INVOCATION=%d\n", 
                saftest_msg_get_ubit64_value(
                    reply, "ASYNC_UNLOCK_INVOCATION"));
    saftest_log("LOCK_STATUS=%s\n", 
                saftest_msg_get_str_value(reply, "LOCK_STATUS"));
    saftest_log("ASYNC_UNLOCK_ERROR_STATUS=%d\n", 
                saftest_msg_get_ubit32_value(reply, 
                                             "ASYNC_UNLOCK_ERROR_STATUS"));
    saftest_msg_free(&reply);
    return(status);
}

SAFTEST_MAP_TABLE_BEGIN(LCK)
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
    "LOCK_RESOURCE_OPEN_REQ", "LOCK_RESOURCE_OPEN_REPLY",
    saftest_client_generic_handle_request,
    saftest_daemon_handle_lck_session_open_request)

SAFTEST_MAP_TABLE_ENTRY(
    "LOCK_RESOURCE_OPEN_ASYNC_REQ", "LOCK_RESOURCE_OPEN_ASYNC_REPLY",
    saftest_client_generic_handle_request,
    saftest_daemon_handle_lck_resource_open_async_request)

SAFTEST_MAP_TABLE_ENTRY(
    "LOCK_RESOURCE_CLOSE_REQ", "LOCK_RESOURCE_CLOSE_REPLY",
    saftest_client_generic_handle_request,
    saftest_daemon_handle_lck_resource_close_request)

SAFTEST_MAP_TABLE_ENTRY(
    "LOCK_SYNC_REQ", "LOCK_SYNC_REPLY",
    saftest_client_generic_handle_request,
    saftest_daemon_handle_sync_lock_request)

SAFTEST_MAP_TABLE_ENTRY(
    "LOCK_ASYNC_REQ", "LOCK_ASYNC_REPLY",
    saftest_client_generic_handle_request,
    saftest_daemon_handle_async_lock_request)

SAFTEST_MAP_TABLE_ENTRY(
    "UNLOCK_SYNC_REQ", "UNLOCK_SYNC_REPLY",
    saftest_client_generic_handle_request,
    saftest_daemon_handle_sync_unlock_request)

SAFTEST_MAP_TABLE_ENTRY(
    "UNLOCK_ASYNC_REQ", "UNLOCK_ASYNC_REPLY",
    saftest_client_generic_handle_request,
    saftest_daemon_handle_async_unlock_request)

SAFTEST_MAP_TABLE_ENTRY(
    "LOCK_GET_WAIT_COUNT_REQ", "LOCK_GET_WAIT_COUNT_REPLY",
    saftest_client_handle_lock_get_wait_count_request,
    saftest_daemon_handle_lock_get_wait_count_request)

SAFTEST_MAP_TABLE_ENTRY(
    "LOCK_GET_ASYNC_LOCK_STATUS_REQ", "LOCK_GET_ASYNC_LOCK_STATUS_REPLY",
    saftest_client_handle_lock_get_async_lock_status_request,
    saftest_daemon_handle_lock_get_async_lock_status_request)

SAFTEST_MAP_TABLE_ENTRY(
    "LOCK_GET_RESOURCE_OPEN_ASYNC_STATUS_REQ", "LOCK_GET_RESOURCE_OPEN_ASYNC_STATUS_REPLY",
    saftest_client_handle_lock_get_resource_open_async_status_request,
    saftest_daemon_handle_lock_get_resource_open_async_status_request)

SAFTEST_MAP_TABLE_ENTRY(
    "LOCK_GET_ASYNC_UNLOCK_STATUS_REQ", "LOCK_GET_ASYNC_UNLOCK_STATUS_REPLY",
    saftest_client_handle_lock_get_async_unlock_status_request,
    saftest_daemon_handle_lock_get_async_unlock_status_request)

SAFTEST_MAP_TABLE_END(LCK)

const char *get_library_id()
{
    return "LCK";
}
