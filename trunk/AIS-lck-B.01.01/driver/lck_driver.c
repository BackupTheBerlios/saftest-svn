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

typedef struct lock_resource {
    int lock_resource_id;
    pthread_t thread_id;

    SaVersionT version;
    SaNameT lock_name;
    SaTimeT timeout;
    SaDispatchFlagsT dispatch_flags;
    SaLckHandleT lck_handle;
    SaSelectionObjectT selection_object;
    SaLckResourceHandleT res_handle;
    SaLckLockIdT lock_id;
    SaLckLockModeT lock_mode;
    SaLckCallbacksT lck_callbacks;
    SaLckResourceOpenFlagsT lock_open_flags;
    SaLckLockStatusT lock_status;
    SaInvocationT async_lock_invocation;
    SaAisErrorT async_lock_error_status;
    SaInvocationT async_unlock_invocation;
    SaAisErrorT async_unlock_error_status;

    SaLckWaiterSignalT last_delivered_waiter_signal;
    ubit32 waiter_signal_notification_count;
} lock_resource_t;

GList *lock_list = NULL;

const char *get_library_id()
{
    return "LCK";
}

void saftest_daemon_init(FILE *log_fp)
{
    assert(NULL != log_fp);
    saftest_log_set_fp(log_fp);
}

static const char *
saftest_map_lock_status_to_string(SaLckLockStatusT op)
{
    if (op == SA_LCK_LOCK_GRANTED) {
        return "SA_LCK_LOCK_GRANTED";
    } else if (op == SA_LCK_LOCK_DEADLOCK) {
        return "SA_LCK_LOCK_DEADLOCK";
    } else if (op == SA_LCK_LOCK_NOT_QUEUED) {
        return "SA_LCK_LOCK_NOT_QUEUED";
    } else if (op == SA_LCK_LOCK_TIMED_OUT) {
        return "SA_LCK_LOCK_TIMED_OUT";
    } else if (op == SA_LCK_LOCK_ORPHANED) {
        return "SA_LCK_LOCK_ORPHANED";
    } else if (op == SA_LCK_LOCK_NO_MORE) {
        return "SA_LCK_LOCK_NO_MORE";
    } else if (op == SA_LCK_LOCK_DUPLICATE_EX) {
        return "SA_LCK_LOCK_DUPLICATE_EX";
    }
    saftest_abort("Unknown op status %d\n", op);
    return "Invalid Lock Status";
}

int
get_next_lock_resource_id()
{
    static int next_lock_resource_id = 1;
    int ret_id;

    ret_id = next_lock_resource_id;
    next_lock_resource_id += 1;
    return(ret_id);
}

lock_resource_t *
add_lock_resource()
{
    lock_resource_t *res;

    res = malloc(sizeof(lock_resource_t));
    assert(NULL != res);
    memset(res, 0, sizeof(lock_resource_t));

    lock_list = g_list_append(lock_list, res);
    res->lock_resource_id = get_next_lock_resource_id();
    saftest_log("Added a lock resource with id %d\n", res->lock_resource_id);
    return(res);
}

void
delete_lock_resource(lock_resource_t *res)
{
    saftest_log("Deleting lock resource with id %d\n", res->lock_resource_id);
    lock_list = g_list_remove(lock_list, res);
    free(res);
}

lock_resource_t *
lookup_lock_resource(int lock_resource_id)
{
    GList *element;
    lock_resource_t *res;

    for (element = g_list_first(lock_list);
         NULL != element;
         element = g_list_next(element)) {
        res = (lock_resource_t *)element->data;
        if (res->lock_resource_id == lock_resource_id) {
            return(res);
        }
    }
    return(NULL);
}

lock_resource_t *
lookup_lock_resource_by_lock_id(SaLckLockIdT lock_id)
{
    GList *element;
    lock_resource_t *res;

    for (element = g_list_first(lock_list);
         NULL != element;
         element = g_list_next(element)) {
        res = (lock_resource_t *)element->data;
        if (res->lock_id == lock_id) {
            return(res);
        }
    }
    return(NULL);
}

lock_resource_t *
lookup_lock_resource_by_lock_invocation(SaInvocationT invocation)
{
    GList *element;
    lock_resource_t *res;

    for (element = g_list_first(lock_list);
         NULL != element;
         element = g_list_next(element)) {
        res = (lock_resource_t *)element->data;
        if (res->async_lock_invocation == invocation) {
            return(res);
        }
    }
    return(NULL);
}

lock_resource_t *
lookup_lock_resource_by_unlock_invocation(SaInvocationT invocation)
{
    GList *element;
    lock_resource_t *res;

    for (element = g_list_first(lock_list);
         NULL != element;
         element = g_list_next(element)) {
        res = (lock_resource_t *)element->data;
        if (res->async_unlock_invocation == invocation) {
            return(res);
        }
    }
    return(NULL);
}

lock_resource_t *
lookup_lock_resource_from_request(saftest_msg_t *request)
{
    lock_resource_t *res;

    res = lookup_lock_resource(
                  saftest_msg_get_ubit32_value(request, "LCK_RESOURCE_ID"));
    if (NULL == res) {
        saftest_abort("Unknown resource id %d\n",
                      saftest_msg_get_ubit32_value(request, "LCK_RESOURCE_ID"));
    }
    return(res);
}

void
saftest_daemon_lock_waiter_callback(SaLckWaiterSignalT waiterSignal,
                                     SaLckLockIdT lockId,
                                     SaLckLockModeT modeHeld,
                                     SaLckLockModeT modeRequested)
{
    lock_resource_t *lock_res;

    lock_res = lookup_lock_resource_by_lock_id(lockId);
    if (NULL == lock_res) {
        saftest_abort("Unknown lock id %d\n", lockId);
    }
    lock_res->last_delivered_waiter_signal = waiterSignal;
    lock_res->waiter_signal_notification_count += 1;
    saftest_log("Received lock waiter callback with signal %lld. "
                 "Notification Count is now %d\n",
                 waiterSignal, lock_res->waiter_signal_notification_count);
}

void
saftest_daemon_lock_grant_callback(SaInvocationT invocation,
                                    SaLckLockStatusT lockStatus,
                                    SaAisErrorT error)
{
    lock_resource_t *lock_res;

    lock_res = lookup_lock_resource_by_lock_invocation(invocation);
    if (NULL == lock_res) {
        saftest_abort("Unknown async lock invocation %d\n", invocation);
    }
    saftest_log("Received lock grant notification for invocation %lld\n",
                 invocation);
    lock_res->lock_status = lockStatus;
    lock_res->async_lock_error_status = error;
}

void
saftest_daemon_lock_release_callback(SaInvocationT invocation,
                                      SaAisErrorT error)
{
    lock_resource_t *lock_res;

    lock_res = lookup_lock_resource_by_unlock_invocation(invocation);
    if (NULL == lock_res) {
        saftest_abort("Unknown async unlock invocation %d\n", invocation);
    }
    saftest_log("Received lock grant notification for invocation %lld\n",
                 invocation);
    lock_res->lock_status = SA_LCK_LOCK_NO_MORE;
    lock_res->async_unlock_error_status = error;
    lock_res->lock_id = 0;
    lock_res->last_delivered_waiter_signal = 0;
    lock_res->waiter_signal_notification_count = 0;
}

static void *
saftest_daemon_dispatch_thread(void *arg)
{
    lock_resource_t *lock_res = (lock_resource_t *)arg;
    SaAisErrorT err;
    
    err = saLckDispatch(lock_res->lck_handle, SA_DISPATCH_BLOCKING);
    if (err != SA_AIS_OK) {
        saftest_abort("Error %s calling "
                       "saLckDispatch(SA_DISPATCH_BLOCKING)\n",
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
    lock_resource_t *lock_res;

    saftest_log("Received a create test resource request.\n");

    lock_res = add_lock_resource();

    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op,
                                        SA_AIS_OK);
    saftest_msg_set_ubit32_value((*reply), "LCK_RESOURCE_ID",
                                 lock_res->lock_resource_id);
}

void
saftest_daemon_handle_init_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lock_resource_t *lock_res;
    int err;
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
                 saftest_msg_get_ubit32_value(request, "LCK_RESOURCE_ID"),
                 releaseCode,
                 saftest_msg_get_ubit32_value(request, "VERSION_MAJOR"),
                 saftest_msg_get_ubit32_value(request, "VERSION_MINOR"));

    lock_res = lookup_lock_resource_from_request(request);
    lock_res->lck_callbacks.saLckResourceOpenCallback = NULL;
    lock_res->lck_callbacks.saLckLockWaiterCallback = NULL;
    lock_res->lck_callbacks.saLckLockGrantCallback = NULL;
    lock_res->lck_callbacks.saLckResourceUnlockCallback = NULL;

    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_LCK_HANDLE"))) {
        handle = &lock_res->lck_handle;
    }
    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_CALLBACKS"))) {
        callbacks = &lock_res->lck_callbacks;
        if (0 == strcmp("TRUE",
                        saftest_msg_get_str_value(request,
                                                  "LOCK_WAITER_CB"))) {
            lock_res->lck_callbacks.saLckLockWaiterCallback =
                saftest_daemon_lock_waiter_callback;
        }
        if (0 == strcmp("TRUE",
                        saftest_msg_get_str_value(request,
                                                  "RESOURCE_UNLOCK_CB"))) {
            lock_res->lck_callbacks.saLckResourceUnlockCallback = 
                saftest_daemon_lock_release_callback;
        }
        if (0 == strcmp("TRUE",
                        saftest_msg_get_str_value(request,
                                                  "LOCK_GRANT_CB"))) {
            lock_res->lck_callbacks.saLckLockGrantCallback = 
                saftest_daemon_lock_grant_callback;
        }
    }
    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_VERSION"))) {
        version = &lock_res->version;
        lock_res->version.releaseCode = releaseCode;
        lock_res->version.majorVersion =
            saftest_msg_get_ubit8_value(request, "VERSION_MAJOR");
        lock_res->version.minorVersion =
            saftest_msg_get_ubit8_value(request, "VERSION_MINOR");
    }

    lock_res->dispatch_flags =
        saftest_daemon_get_dispatch_flags(
                   saftest_msg_get_str_value(request, "DISPATCH_FLAGS"));

    status = saLckInitialize(handle, callbacks, version);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
    if (SA_AIS_OK == status) {
        if (SA_DISPATCH_BLOCKING == lock_res->dispatch_flags) {
            saftest_log("Starting new dispatch thread\n");
            err = pthread_create(&lock_res->thread_id, NULL,
                                 saftest_daemon_dispatch_thread,
                                 (void*)lock_res);
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
    lock_resource_t *lock_res = NULL;
    SaSelectionObjectT *selection_object = NULL;
    SaAisErrorT status;

    saftest_log("Received a select object get request for id %d\n",
                saftest_msg_get_ubit32_value(request, "LCK_RESOURCE_ID"));
    lock_res = lookup_lock_resource_from_request(request);

    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request,
                                              "NULL_SELECTION_OBJECT"))) {
        selection_object = &lock_res->selection_object;
    }
    status = saLckSelectionObjectGet(lock_res->lck_handle, selection_object);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_dispatch_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lock_resource_t *lock_res = NULL;
    SaAisErrorT status;
    SaDispatchFlagsT dispatch_flags;

    saftest_log("Received a dispatch request for id %d\n",
                saftest_msg_get_ubit32_value(request, "LCK_RESOURCE_ID"));
    lock_res = lookup_lock_resource_from_request(request);

    dispatch_flags =
        saftest_daemon_get_dispatch_flags(
                   saftest_msg_get_str_value(request, "DISPATCH_FLAGS"));
    saftest_assert(SA_DISPATCH_BLOCKING != dispatch_flags,
                   "Can't use blocking dispatch for a dispatch request\n");
    status = saLckDispatch(lock_res->lck_handle, dispatch_flags);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void
saftest_daemon_handle_resource_finalize_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lock_resource_t *lock_res = NULL;
    SaAisErrorT status;

    saftest_log("Received a finalize request for id %d\n",
                saftest_msg_get_ubit32_value(request, "LCK_RESOURCE_ID"));
    lock_res = lookup_lock_resource_from_request(request);

    status = saLckFinalize(lock_res->lck_handle);
    lock_res->selection_object = 0;
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_lock_resource_open_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lock_resource_t *lock_res = NULL;
    SaAisErrorT status;

    saftest_log("Received a resource open request for id %d name %s\n",
                saftest_msg_get_ubit32_value(request, "LCK_RESOURCE_ID"),
                saftest_msg_get_str_value(request, "LOCK_NAME"));
    
    lock_res = lookup_lock_resource_from_request(request);

    lock_res->lock_open_flags |= SA_LCK_RESOURCE_CREATE;
    lock_res->lock_name.length = 
        strlen(saftest_msg_get_str_value(request, "LOCK_NAME")) + 1;
    strcpy(lock_res->lock_name.value,
           saftest_msg_get_str_value(request, "LOCK_NAME"));
    status = saLckResourceOpen(lock_res->lck_handle, 
                               &lock_res->lock_name, 
                               lock_res->lock_open_flags,
                               0, &lock_res->res_handle);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_lock_resource_close_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lock_resource_t *lock_res = NULL;
    SaAisErrorT status;

    saftest_log("Received a resource close request for id %d\n",
                saftest_msg_get_ubit32_value(request, "LCK_RESOURCE_ID"));
    lock_res = lookup_lock_resource_from_request(request);
    status = saLckResourceClose(lock_res->res_handle);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_sync_lock_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lock_resource_t *lock_res = NULL;
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

    saftest_log("Received a sync lock request with flags %x lock_mode %d "
                 "null_lock_id %s waiter_signal %lld for id %d\n",
                 lock_flags, lock_mode,
                 saftest_msg_get_str_value(request, "NULL_LOCK_ID"),
                 waiter_signal,
                 saftest_msg_get_ubit32_value(request, "LCK_RESOURCE_ID"));
    lock_res = lookup_lock_resource_from_request(request);

    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_LOCK_ID"))) {
        lock_id = &lock_res->lock_id;
    }
    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_LOCK_STATUS"))) {
        lock_status = &lock_res->lock_status;
    }
    status = saLckResourceLock(lock_res->lck_handle,
                            lock_id,
                            lock_mode,
                            lock_flags,
                            waiter_signal, timeout,
                            lock_status);
    saftest_log("saLckResourceLock returned %d, lock_status is %d\n",
                 status, lock_res->lock_status);
    if (status == SA_AIS_OK) {
        if (0 ==
            strcmp(saftest_map_lock_status_to_string(lock_res->lock_status), 
                   saftest_msg_get_str_value(request, 
                                             "EXPECTED_LOCK_STATUS"))) {
            status = SA_AIS_OK;
        } else {
            saftest_log("Expected lock status %s but got status %d\n",
                        saftest_msg_get_str_value(request, 
                                                  "EXPECTED_LOCK_STATUS"),
                        saftest_map_lock_status_to_string(
                            lock_res->lock_status));
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
    lock_resource_t *lock_res = NULL;
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
                 saftest_msg_get_ubit32_value(request, "LCK_RESOURCE_ID"));
    lock_res = lookup_lock_resource_from_request(request);
    if (0 == strcmp("FALSE",
                    saftest_msg_get_str_value(request, "NULL_LOCK_ID"))) {
        lock_id = &lock_res->lock_id;
    }
    lock_res->async_lock_invocation = invocation;
    status = saLckResourceLockAsync(lock_res->lck_handle, invocation, lock_id, 
                                    lock_mode, lock_flags, waiter_signal);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_sync_unlock_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lock_resource_t *lock_res = NULL;
    SaTimeT timeout = 0;
    SaAisErrorT status;

    saftest_log("Received a blocking unlock request for id %d\n",
                 saftest_msg_get_ubit32_value(request, "LCK_RESOURCE_ID"));
    lock_res = lookup_lock_resource_from_request(request);
    status = saLckResourceUnlock(lock_res->lock_id, timeout);
    if (SA_AIS_OK == status) {
        lock_res->lock_id = 0;
        lock_res->last_delivered_waiter_signal = 0;
        lock_res->waiter_signal_notification_count = 0;
        lock_res->lock_status = SA_LCK_LOCK_NO_MORE;
    }
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_async_unlock_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lock_resource_t *lock_res = NULL;
    SaInvocationT invocation = 0;
    SaAisErrorT status;

    invocation = saftest_msg_get_ubit64_value(request, "INVOCATION");
    saftest_log("Received an async unlock request for id %d invocation %lld\n",
                saftest_msg_get_ubit32_value(request, "LCK_RESOURCE_ID"),
                invocation);
    lock_res = lookup_lock_resource_from_request(request);
    lock_res->async_unlock_invocation = invocation;
    status = saLckResourceUnlockAsync(invocation, lock_res->lock_id);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, status);
}

void 
saftest_daemon_handle_lock_get_wait_count_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lock_resource_t *lock_res = NULL;

    saftest_log("Received a signal notify count request for id %d\n",
                saftest_msg_get_ubit32_value(request, "LCK_RESOURCE_ID"));
    lock_res = lookup_lock_resource_from_request(request);

    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        SA_AIS_OK);
    saftest_msg_set_ubit64_value(*reply, "LAST_DELIVERED_WAITER_SIGNAL",
                                 lock_res->last_delivered_waiter_signal);
    saftest_msg_set_ubit32_value(*reply, "WAITER_SIGNAL_NOTIFICATION_COUNT",
                                 lock_res->waiter_signal_notification_count);
}

void 
saftest_daemon_handle_lock_get_async_lock_status_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lock_resource_t *lock_res = NULL;

    saftest_log("Received an async lock status request for id %d\n",
                saftest_msg_get_ubit32_value(request, "LCK_RESOURCE_ID"));
    lock_res = lookup_lock_resource_from_request(request);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        lock_res->async_lock_error_status);
    saftest_msg_set_ubit64_value(*reply, "LAST_DELIVERED_WAITER_SIGNAL",
                                 lock_res->last_delivered_waiter_signal);
    saftest_msg_set_str_value(
        *reply, "LOCK_STATUS",
        saftest_map_lock_status_to_string(lock_res->lock_status));
    saftest_msg_set_ubit64_value(*reply, "ASYNC_LOCK_INVOCATION",
                                 lock_res->async_lock_invocation);
}

void 
saftest_daemon_handle_unlock_get_async_lock_status_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    lock_resource_t *lock_res = NULL;

    saftest_log("Received an async unlock status request for id %d\n",
                saftest_msg_get_ubit32_value(request, "LCK_RESOURCE_ID"));
    lock_res = lookup_lock_resource_from_request(request);
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 
                                        lock_res->async_unlock_error_status);
    saftest_msg_set_str_value(
        *reply, "LOCK_STATUS",
        saftest_map_lock_status_to_string(lock_res->lock_status));
    saftest_msg_set_ubit64_value(*reply, "ASYNC_UNLOCK_INVOCATION",
                                 lock_res->async_unlock_invocation);
}

void
saftest_daemon_handle_incoming_lock_message(gpointer data, gpointer user_data)
{
    lock_resource_t *lock_res = data;
    fd_set *fd_mask = (fd_set *)user_data;
    SaAisErrorT err;

    if (NULL == data) {
        return;
    }

    if (!FD_ISSET(lock_res->selection_object, fd_mask)) {
        return;
    }

    saftest_log("Incoming request on lock selection fd %d\n",
                 lock_res->selection_object);
    saftest_assert(SA_DISPATCH_BLOCKING != lock_res->dispatch_flags,
                   "It will have its own dispatch thread\n");
    err = saLckDispatch(lock_res->lck_handle, lock_res->dispatch_flags);
    if (SA_AIS_OK != err) {
        saftest_log("Error %s performing saLckDispatch\n",
                     get_error_string(err));
    }
}

void saftest_daemon_add_lock_resource_to_fdset(gpointer data, 
                                                gpointer user_data)
{
    lock_resource_t *lock_res;
    fd_set_key_t *set_key = (fd_set_key_t *)user_data;

    if (NULL == data) {
        return;
    }

    lock_res = (lock_resource_t *)data;
    if (lock_res->thread_id > 0) {
        /* It will have its own dispatch thread */
        return;
    }

    if (lock_res->selection_object > 0) {
        FD_SET(lock_res->selection_object, set_key->set);
        if (lock_res->selection_object > set_key->largest_fd) {
            set_key->largest_fd = lock_res->selection_object;
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

    g_list_foreach(lock_list, saftest_daemon_add_lock_resource_to_fdset,
                   &set_key);
    *max_fd = set_key.largest_fd;
}

void
saftest_daemon_check_fds(
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    g_list_foreach(lock_list,
                   saftest_daemon_handle_incoming_lock_message,
                   read_fd_set);
}

/*
int
saftest_client_send_sync_lock_request(saftest_lck_request_op_t op,
                                           const char *lock_name)
{
    strcpy(request->lock_name, lock_name);

*/

SaAisErrorT
saftest_client_handle_create_test_res_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply;
    SaAisErrorT status;
 
    saftest_send_request(fd, get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");
    status = saftest_reply_msg_get_status(reply);

    if (SA_AIS_OK == status) {
        saftest_log("Resource ID=%d\n",
                    saftest_msg_get_ubit32_value(reply,
                                                 "LCK_RESOURCE_ID"));
    }
    saftest_msg_free(&reply);
    return(status);
}

SaAisErrorT
saftest_client_handle_lock_get_wait_count_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply;
    SaAisErrorT status;
 
    saftest_send_request(fd, get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");
    status = saftest_reply_msg_get_status(reply);

    if (SA_AIS_OK == status) {
        saftest_log("Last Delivered Lock Waiter Signal=%lld\n", 
                    saftest_msg_get_ubit64_value(
                        reply, "LAST_DELIVERED_WAITER_SIGNAL"));
        saftest_log("Lock Waiter Notification Count=%d\n", 
                    saftest_msg_get_ubit32_value(
                        reply, "WAITER_SIGNAL_NOTIFICATION_COUNT"));
    }
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
 
    saftest_send_request(fd, get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");
    status = saftest_reply_msg_get_status(reply);

    saftest_log("Async Lock Invocation=%d\n", 
                saftest_msg_get_ubit64_value(
                    reply, "ASYNC_LOCK_INVOCATION"));
    saftest_log("Async Lock Status=%s\n", 
                saftest_msg_get_str_value(reply, "LOCK_STATUS"));
    saftest_msg_free(&reply);
    return(status);
}

SaAisErrorT
saftest_client_handle_unlock_get_async_lock_status_request(
    int fd,
    saftest_msg_t *request)
{
    saftest_msg_t *reply;
    SaAisErrorT status;
 
    saftest_send_request(fd, get_library_id(), request, &reply);
    saftest_assert(NULL != reply, "Received no reply from the daemon\n");
    status = saftest_reply_msg_get_status(reply);

    saftest_log("Async Unlock Invocation=%d\n", 
                saftest_msg_get_ubit64_value(
                    reply, "ASYNC_UNLOCK_INVOCATION"));
    saftest_log("Async Unlock Status=%s\n", 
                saftest_msg_get_str_value(reply, "LOCK_STATUS"));
    saftest_msg_free(&reply);
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
    "LOCK_RESOURCE_OPEN_REQ", "LOCK_RESOURCE_OPEN_REPLY",
    saftest_client_generic_handle_request,
    saftest_daemon_handle_lock_resource_open_request)

/*
SAFTEST_MAP_TABLE_ENTRY(
    "LOCK_RESOURCE_OPEN_ASYNC_REQ", "LOCK_RESOURCE_OPEN_ASYNC_REPLY",
    saftest_client_generic_handle_request,
    saftest_daemon_handle_lock_resource_open_async_request)
*/

SAFTEST_MAP_TABLE_ENTRY(
    "LOCK_RESOURCE_CLOSE_REQ", "LOCK_RESOURCE_CLOSE_REPLY",
    saftest_client_generic_handle_request,
    saftest_daemon_handle_lock_resource_close_request)

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
    "LOCK_GET_ASYNC_UNLOCK_STATUS_REQ", "LOCK_GET_ASYNC_UNLOCK_STATUS_REPLY",
    saftest_client_handle_unlock_get_async_lock_status_request,
    saftest_daemon_handle_unlock_get_async_lock_status_request)

SAFTEST_MAP_TABLE_END()
