/**********************************************************************
 *
 *	I N C L U D E S
 *
 **********************************************************************/
#include "saftest_driver_lib_utils.h"
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

enum ais_test_lck_request_op {
    AIS_TEST_LCK_REQUEST_INVALID=0,
    AIS_TEST_LCK_REQUEST_CREATE_TEST_RESOURCE,
    AIS_TEST_LCK_REQUEST_LOCK_INITALIZE,
    AIS_TEST_LCK_REQUEST_LOCK_SELECTION_OBJECT_GET,
    AIS_TEST_LCK_REQUEST_LOCK_RESOURCE_OPEN,
    AIS_TEST_LCK_REQUEST_LOCK_RESOURCE_OPEN_ASYNC,
    AIS_TEST_LCK_REQUEST_LOCK_RESOURCE_CLOSE,
    AIS_TEST_LCK_REQUEST_LOCK_FINALIZE,
    AIS_TEST_LCK_REQUEST_LOCK_SYNC,
    AIS_TEST_LCK_REQUEST_LOCK_ASYNC,
    AIS_TEST_LCK_REQUEST_UNLOCK_SYNC,
    AIS_TEST_LCK_REQUEST_UNLOCK_ASYNC,
    AIS_TEST_LCK_REQUEST_DISPATCH,
    AIS_TEST_LCK_REQUEST_WAITER_SIGNAL_NOTIFY_COUNT,
    AIS_TEST_LCK_REQUEST_ASYNC_LOCK_STATUS,
    AIS_TEST_LCK_REQUEST_ASYNC_UNLOCK_STATUS,
};
typedef enum ais_test_lck_request_op ais_test_lck_request_op_t;

/*
 * op - Used for all requests
 * requestor_pid - Used for all requests
 * release_code, major_version, minor_version - Used by LOCK_INITIALIZE
 * resource_open_cb_flag - Used by LOCK_INITIALIZE
 * lock_grant_cb_flag - Used by LOCK_INITIALIZE
 * lock_waiter_cb_flag - Used by LOCK_INITIALIZE
 * resource_unlock_cb_flag - Used by LOCK_INITIALIZE
 * lock_name - Used by RESOURCE_OPEN, RESOURCE_OPEN_ASYNC
 * lock_resource_id - Used by RESOURCE_OPEN, RESOURCE_OPEN_ASYNC,
 *                    RESOURCE_CLOSE, LOCK_SYNC, LOCK_ASYNC, UNLOCK
 * lock_mode - Used by LOCK_SYNC, LOCK_ASYNC
 */

typedef struct ais_test_lck_request {
    ais_test_lck_request_op_t op;
    pid_t requestor_pid;
    SaVersionT sa_version;
    int resource_open_cb_flag;
    int lock_grant_cb_flag;
    int lock_waiter_cb_flag;
    int resource_unlock_cb_flag;
    SaDispatchFlagsT dispatch_flags;
    SaLckWaiterSignalT waiter_signal;
    SaInvocationT invocation;
    int lock_resource_id;
    char lock_name[BUF_SIZE];
    SaLckLockModeT lock_mode;
    SaLckLockFlagsT lock_flags;
    int expected_lock_status;
    int null_lock_handle_flag;
    int null_callbacks_flag;
    int null_version_flag;
    int null_lock_id_flag;
    int null_lock_status_flag;
    int null_selection_object_flag;
} ais_test_lck_request_t;

/*
 * status - always valid
 * lock_resource_id - Only valid when request op was 
 *                    AIS_TEST_LCK_REQUEST_LOCK_INITALIZE
 */
typedef struct ais_test_lck_reply {
    SaAisErrorT status;
    int lock_resource_id;
    SaLckWaiterSignalT last_delivered_waiter_signal;
    int waiter_signal_notification_count;
    SaLckLockStatusT lock_status;
    SaInvocationT async_lock_invocation;
    SaInvocationT async_unlock_invocation;
} ais_test_lck_reply_t;

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
    int waiter_signal_notification_count;
} lock_resource_t;

GList *lock_list = NULL;

void
usage()
{
    printf("Server Usage: lck_driver --daemon\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --log-file <log file>\n");
    printf("                         --pid-file <pid file>\n");
    printf("                        [-no-daemonize]\n");

    printf("\n");

    printf("Client Usage: lck_driver --o CREATE_TEST_RES\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("\n");
    printf("Client Usage: lck_driver --o INIT\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --dispatch-flags \n");
    printf("                           <SA_LCK_DISPATCH_BLOCKING |\n");
    printf("                            SA_LCK_DISPATCH_ONE |\n");
    printf("                            SA_LCK_DISPATCH_ALL>  \n");
    printf("                        [--set-resource-open-cb]\n");
    printf("                        [--set-lock-grant-cb]\n");
    printf("                        [--set-lock-waiter-cb]\n");
    printf("                          --waiter-signal <signal #>]\n");
    printf("                        [--set-resource-unlock-cb]\n");
    printf("                        [--version-release-code <code #>]\n");
    printf("                        [--version-major <code #>]\n");
    printf("                        [--version-minor <code #>]\n");
    printf("                        [--null-lck-handle]\n");
    printf("                        [--null-callbacks]\n");
    printf("                        [--null-version]\n");
    printf("\n");
    printf("Client Usage: lck_driver --o SELECT_OBJ_GET\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                        [--null-selection-object]\n");
    printf("\n");
    printf("Client Usage: lck_driver --o RES_OPEN\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                         --lock-name <Lock Name>\n");
    printf("                        [--timeout <timeout>]\n");
    printf("\n");
    printf("Client Usage: lck_driver --o RES_OPEN_ASYNC\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                         --lock-name <Lock Name>\n");
    printf("\n");
    printf("Client Usage: lck_driver --o RES_CLOSE\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("\n");
    printf("Client Usage: lck_driver --o FINALIZE\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("\n");
    printf("Client Usage: lck_driver --o LOCK_SYNC\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                         --lock-mode <PR | EX>\n");
    printf("                         --expected-status <status code>\n");
    printf("                        [--lock-flag-no-queue]\n");
    printf("                        [--lock-flag-orphan]\n");
    printf("                        [--lock-flag-invalid]\n");
    printf("                        [--null-lock-id]\n");
    printf("                        [--null-lock-status]\n");
    printf("                        [--timeout <timeout>]\n");
    printf("\n");
    printf("Client Usage: lck_driver --o LOCK_ASYNC\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                         --lock-mode <PR | EX>\n");
    printf("                         --invocation <invocation>\n");
    printf("                         --expected-status <status code>\n");
    printf("                        [--lock-flag-no-queue]\n");
    printf("                        [--lock-flag-orphan]\n");
    printf("                        [--lock-flag-invalid]\n");
    printf("                        [--null-lock-id]\n");
    printf("\n");
    printf("Client Usage: lck_driver --o UNLOCK_SYNC\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                        [--timeout <timeout>]\n");
    printf("\n");
    printf("Client Usage: lck_driver --o UNLOCK_ASYNC\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                        [--timeout <timeout>]\n");
    printf("\n");
    printf("Client Usage: lck_driver --o DISPATCH\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("                         --dispatch-flags \n");
    printf("                           <SA_LCK_DISPATCH_BLOCKING |\n");
    printf("                            SA_LCK_DISPATCH_ONE |\n");
    printf("                            SA_LCK_DISPATCH_ALL>  \n");
    printf("\n");
    printf("Client Usage: lck_driver --o WAIT_COUNT\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("\n");
    printf("Client Usage: lck_driver --o ASYNC_LOCK_STATUS\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("\n");
    printf("Client Usage: lck_driver --o ASYNC_UNLOCK_STATUS\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --resource-id <resource id>\n");
    printf("\n");
	exit(255);
}

const char *get_library_id()
{
    return "LCK";
}

int get_library_message_size()
{
    return sizeof(ais_test_lck_request_t);
}

void saftest_daemon_init(FILE *log_fp)
{
    assert(NULL != log_fp);
    ais_test_log_set_fp(log_fp);
}

static ais_test_lck_request_op_t
ais_test_map_string_to_op(const char *str)
{
    if (0 == strcmp(str, "CREATE_TEST_RES")) {
        return AIS_TEST_LCK_REQUEST_CREATE_TEST_RESOURCE;
    } else if (0 == strcmp(str, "INIT")) {
        return AIS_TEST_LCK_REQUEST_LOCK_INITALIZE;
    } else if (0 == strcmp(str, "SELECT_OBJ_GET")) {
        return AIS_TEST_LCK_REQUEST_LOCK_SELECTION_OBJECT_GET;
    } else if (0 == strcmp(str, "RES_OPEN")) {
        return AIS_TEST_LCK_REQUEST_LOCK_RESOURCE_OPEN;
    } else if (0 == strcmp(str, "RES_OPEN_ASYNC")) {
        return AIS_TEST_LCK_REQUEST_LOCK_RESOURCE_OPEN_ASYNC;
    } else if (0 == strcmp(str, "RES_CLOSE")) {
        return AIS_TEST_LCK_REQUEST_LOCK_RESOURCE_CLOSE;
    } else if (0 == strcmp(str, "FINALIZE")) {
        return AIS_TEST_LCK_REQUEST_LOCK_FINALIZE;
    } else if (0 == strcmp(str, "LOCK_SYNC")) {
        return AIS_TEST_LCK_REQUEST_LOCK_SYNC;
    } else if (0 == strcmp(str, "LOCK_ASYNC")) {
        return AIS_TEST_LCK_REQUEST_LOCK_ASYNC;
    } else if (0 == strcmp(str, "UNLOCK_SYNC")) {
        return AIS_TEST_LCK_REQUEST_UNLOCK_SYNC;
    } else if (0 == strcmp(str, "UNLOCK_ASYNC")) {
        return AIS_TEST_LCK_REQUEST_UNLOCK_ASYNC;
    } else if (0 == strcmp(str, "WAIT_COUNT")) {
        return AIS_TEST_LCK_REQUEST_WAITER_SIGNAL_NOTIFY_COUNT;
    } else if (0 == strcmp(str, "ASYNC_LOCK_STATUS")) {
        return AIS_TEST_LCK_REQUEST_ASYNC_LOCK_STATUS;
    } else if (0 == strcmp(str, "ASYNC_UNLOCK_STATUS")) {
        return AIS_TEST_LCK_REQUEST_ASYNC_UNLOCK_STATUS;
    } else if (0 == strcmp(str, "DISPATCH")) {
        return AIS_TEST_LCK_REQUEST_DISPATCH;
    }
    ais_test_abort("Unknown op string %s\n", str);
    return AIS_TEST_LCK_REQUEST_INVALID;
}

static const char *
ais_test_map_lock_status_to_string(ais_test_lck_request_op_t op)
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
    ais_test_abort("Unknown op status %d\n", op);
    return "Invalid Lock Status";
}

int
get_next_lck_resource_id()
{
    static int next_lck_resource_id = 1;
    int ret_id;

    ret_id = next_lck_resource_id;
    next_lck_resource_id += 1;
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
    res->lock_resource_id = get_next_lck_resource_id();
    ais_test_log("Added a lock resource with id %d\n", res->lock_resource_id);
    return(res);
}

void
delete_lock_resource(lock_resource_t *res)
{
    ais_test_log("Deleting lock resource with id %d\n", res->lock_resource_id);
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

void
ais_test_daemon_lock_waiter_callback(SaLckWaiterSignalT waiterSignal,
                                     SaLckLockIdT lockId,
                                     SaLckLockModeT modeHeld,
                                     SaLckLockModeT modeRequested)
{
    lock_resource_t *lock_res;

    lock_res = lookup_lock_resource_by_lock_id(lockId);
    if (NULL == lock_res) {
        ais_test_abort("Unknown lock id %d\n", lockId);
    }
    lock_res->last_delivered_waiter_signal = waiterSignal;
    lock_res->waiter_signal_notification_count += 1;
    ais_test_log("Received lock waiter callback with signal %lld. "
                 "Notification Count is now %d\n",
                 waiterSignal, lock_res->waiter_signal_notification_count);
}

void
ais_test_daemon_lock_grant_callback(SaInvocationT invocation,
                                    SaLckLockStatusT lockStatus,
                                    SaAisErrorT error)
{
    lock_resource_t *lock_res;

    lock_res = lookup_lock_resource_by_lock_invocation(invocation);
    if (NULL == lock_res) {
        ais_test_abort("Unknown async lock invocation %d\n", invocation);
    }
    ais_test_log("Received lock grant notification for invocation %lld\n",
                 invocation);
    lock_res->lock_status = lockStatus;
    lock_res->async_lock_error_status = error;
}

void
ais_test_daemon_lock_release_callback(SaInvocationT invocation,
                                      SaAisErrorT error)
{
    lock_resource_t *lock_res;

    lock_res = lookup_lock_resource_by_unlock_invocation(invocation);
    if (NULL == lock_res) {
        ais_test_abort("Unknown async unlock invocation %d\n", invocation);
    }
    ais_test_log("Received lock grant notification for invocation %lld\n",
                 invocation);
    lock_res->lock_status = SA_LCK_LOCK_NO_MORE;
    lock_res->async_unlock_error_status = error;
    lock_res->lock_id = 0;
    lock_res->last_delivered_waiter_signal = 0;
    lock_res->waiter_signal_notification_count = 0;
}

static void *
ais_test_daemon_dispatch_thread(void *arg)
{
    lock_resource_t *lock_res = (lock_resource_t *)arg;
    SaAisErrorT err;
    
    err = saLckDispatch(lock_res->lck_handle, SA_DISPATCH_BLOCKING);
    if (err != SA_AIS_OK) {
        ais_test_abort("Error %s calling "
                       "saLckDispatch(SA_DISPATCH_BLOCKING)\n",
                       get_error_string(err));
    }
    return(NULL);
}

void 
ais_test_daemon_handle_create_test_res_request(
    ais_test_lck_request_t *request,
    ais_test_lck_reply_t *reply)
{
    lock_resource_t *lock_res;
    
    ais_test_log("Received a create test resource request from pid %d.\n",
                 request->requestor_pid);

    lock_res = add_lock_resource();

    reply->lock_resource_id = lock_res->lock_resource_id;
    reply->status = SA_AIS_OK;
}

void 
ais_test_daemon_handle_init_request(ais_test_lck_request_t *request,
                                    ais_test_lck_reply_t *reply)
{
    lock_resource_t *lock_res;
    int err;
    SaLckHandleT *handle = NULL;
    SaLckCallbacksT *callbacks = NULL;
    SaVersionT *version = NULL;
    
    ais_test_log("Received an init request from pid %d for id %d "
                 "release code=%d, majorVersion=%d, minorVersion=%d\n",
                 request->requestor_pid,
                 request->lock_resource_id,
                 request->sa_version.releaseCode,
                 request->sa_version.majorVersion,
                 request->sa_version.minorVersion);

    lock_res = lookup_lock_resource(request->lock_resource_id);
    if (NULL == lock_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->lock_resource_id);
    }

    if (0 == request->null_lock_handle_flag) {
        handle = &lock_res->lck_handle;
    }
    if (0 == request->null_callbacks_flag) {
        callbacks = &lock_res->lck_callbacks;
    }
    if (0 == request->null_version_flag) {
        version = &lock_res->version;
    }

    lock_res->lck_callbacks.saLckResourceOpenCallback = NULL;
    lock_res->lck_callbacks.saLckLockWaiterCallback = NULL;
    lock_res->lck_callbacks.saLckLockGrantCallback = NULL;
    lock_res->lck_callbacks.saLckResourceUnlockCallback = NULL;
    if (request->lock_grant_cb_flag) {
        lock_res->lck_callbacks.saLckLockGrantCallback = 
            ais_test_daemon_lock_grant_callback;
    }
    if (request->resource_unlock_cb_flag) {
        lock_res->lck_callbacks.saLckResourceUnlockCallback = 
            ais_test_daemon_lock_release_callback;
    }
    if (request->lock_waiter_cb_flag) {
        lock_res->lck_callbacks.saLckLockWaiterCallback =
            ais_test_daemon_lock_waiter_callback;
    }

    lock_res->version = request->sa_version;
    lock_res->dispatch_flags = request->dispatch_flags;

    reply->status = saLckInitialize(handle, callbacks, version);
    ais_test_log("status is %d\n", reply->status);
    if (SA_AIS_OK == reply->status) {
        if (request->dispatch_flags == SA_DISPATCH_BLOCKING) {
            ais_test_log("Starting new dispatch thread\n");
            err = pthread_create(&lock_res->thread_id, NULL, 
                                 ais_test_daemon_dispatch_thread,
                                 (void*)lock_res);
            if (err) {
                ais_test_abort("Error creating new thread: (%d) %s\n", 
                               errno, strerror(errno));
            }
        }
    }
}

void 
ais_test_daemon_handle_selection_object_request(ais_test_lck_request_t *request,
                                                ais_test_lck_reply_t *reply)
{
    lock_resource_t *lock_res = NULL;
    SaSelectionObjectT *selection_object = NULL;

    ais_test_log("Received a select object get request for id %d\n",
                 request->lock_resource_id);
    lock_res = lookup_lock_resource(request->lock_resource_id);
    if (0 == request->null_selection_object_flag) {
        selection_object = &lock_res->selection_object;
    }
    if (NULL == lock_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->lock_resource_id);
    }
    reply->status = saLckSelectionObjectGet(lock_res->lck_handle, 
                                            selection_object);
    if (0 == request->null_selection_object_flag) {
        ais_test_log("New Lock Selection Object on fd %d\n",
                     selection_object);
    }
}

void 
ais_test_daemon_handle_resource_open_request(ais_test_lck_request_t *request,
                                             ais_test_lck_reply_t *reply)
{
    lock_resource_t *lock_res = NULL;

    ais_test_log("Received a resource open request for id %d name %s\n",
                request->lock_resource_id, request->lock_name);
    lock_res = lookup_lock_resource(request->lock_resource_id);
    if (NULL == lock_res) {
        ais_test_abort("Unknown resource id %d\n",
                    request->lock_resource_id);
    }
    lock_res->lock_open_flags |= SA_LCK_RESOURCE_CREATE;
    lock_res->lock_name.length = strlen(request->lock_name)+1;
    strncpy(lock_res->lock_name.value, request->lock_name, 
            lock_res->lock_name.length);
    reply->status = saLckResourceOpen(lock_res->lck_handle, 
                                      &lock_res->lock_name, 
                                      lock_res->lock_open_flags,
                                      0, &lock_res->res_handle);
}

void 
ais_test_daemon_handle_resource_close_request(ais_test_lck_request_t *request,
                                             ais_test_lck_reply_t *reply)
{
    lock_resource_t *lock_res = NULL;

    ais_test_log("Received a resource open request for id %d name %s\n",
                 request->lock_resource_id, request->lock_name);
    lock_res = lookup_lock_resource(request->lock_resource_id);
    if (NULL == lock_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->lock_resource_id);
    }
    reply->status = saLckResourceClose(lock_res->res_handle);
}

void 
ais_test_daemon_handle_finalize_request(ais_test_lck_request_t *request,
                                        ais_test_lck_reply_t *reply)
{
    lock_resource_t *lock_res = NULL;

    ais_test_log("Received a finalize request for id %d name %s\n",
                 request->lock_resource_id, request->lock_name);
    lock_res = lookup_lock_resource(request->lock_resource_id);
    if (NULL == lock_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->lock_resource_id);
    }
    reply->status = saLckFinalize(lock_res->lck_handle);
    lock_res->selection_object = 0;
}

void 
ais_test_daemon_handle_sync_lock_request(ais_test_lck_request_t *request,
                                         ais_test_lck_reply_t *reply)
{
    lock_resource_t *lock_res = NULL;
    SaTimeT timeout = 0;
    SaAisErrorT err;
    SaLckLockIdT *lock_id = NULL;
    SaLckLockStatusT *lock_status = NULL;

    ais_test_log("Received a sync lock request with flags %x lock_mode %d "
                 "null_lock_id %s for id %d name %s\n",
                 request->lock_flags, request->lock_mode,
                 0 == request->null_lock_id_flag ? "FALSE" : "TRUE",
                 request->lock_resource_id, request->lock_name);
    lock_res = lookup_lock_resource(request->lock_resource_id);
    if (NULL == lock_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->lock_resource_id);
    }
    if (0 == request->null_lock_id_flag) {
        lock_id = &lock_res->lock_id;
    }
    if (0 == request->null_lock_status_flag) {
        lock_status = &lock_res->lock_status;
    }
    err = saLckResourceLock(lock_res->lck_handle,
                            lock_id,
                            request->lock_mode,
                            request->lock_flags,
                            request->waiter_signal, timeout,
                            lock_status);
    ais_test_log("saLckResourceLock returned %d, lock_status is %d\n",
                 err, lock_res->lock_status);
    if (err == SA_AIS_OK) {
        if (lock_res->lock_status == request->expected_lock_status) {
            reply->status = SA_AIS_OK;
        } else {
            ais_test_log("Expected lock status %d but got status %d\n",
                         request->expected_lock_status,
                         lock_res->lock_status);
            reply->status = SA_AIS_ERR_LIBRARY;
        }
    } else {
       reply->status = err;
    }
}

void 
ais_test_daemon_handle_async_lock_request(ais_test_lck_request_t *request,
                                             ais_test_lck_reply_t *reply)
{
    lock_resource_t *lock_res = NULL;
    SaLckLockIdT *lock_id = NULL;

    ais_test_log("Received an async lock request with flags %x lock_mode %d "
                 "invocation %lld null_lock_id %s for id %d name %s\n",
                 request->lock_flags, request->lock_mode,
                 request->invocation,
                 0 == request->null_lock_id_flag ? "FALSE" : "TRUE",
                 request->lock_resource_id, request->lock_name);
    lock_res = lookup_lock_resource(request->lock_resource_id);
    if (NULL == lock_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->lock_resource_id);
    }
    if (0 == request->null_lock_id_flag) {
        lock_id = &lock_res->lock_id;
    }
    lock_res->async_lock_invocation = request->invocation;
    reply->status = saLckResourceLockAsync(lock_res->lck_handle,
                                           request->invocation,
                                           lock_id,
                                           request->lock_mode,
                                           request->lock_flags,
                                           request->waiter_signal);
}

void 
ais_test_daemon_handle_sync_unlock_request(ais_test_lck_request_t *request,
                                               ais_test_lck_reply_t *reply)
{
    lock_resource_t *lock_res = NULL;
    SaTimeT timeout = 0;

    ais_test_log("Received a blocking unlock request for id %d name %s\n",
                 request->lock_resource_id, request->lock_name);
    lock_res = lookup_lock_resource(request->lock_resource_id);
    if (NULL == lock_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->lock_resource_id);
    }
    reply->status = saLckResourceUnlock(lock_res->lock_id, timeout);
    if (SA_AIS_OK == reply->status) {
        lock_res->lock_id = 0;
        lock_res->last_delivered_waiter_signal = 0;
        lock_res->waiter_signal_notification_count = 0;
        lock_res->lock_status = SA_LCK_LOCK_NO_MORE;
    }
}

void 
ais_test_daemon_handle_async_unlock_request(ais_test_lck_request_t *request,
                                            ais_test_lck_reply_t *reply)
{
    lock_resource_t *lock_res = NULL;

    ais_test_log("Received an async unlock request for id %d invocation %lld\n",
                 request->lock_resource_id, request->invocation);
    lock_res = lookup_lock_resource(request->lock_resource_id);
    if (NULL == lock_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->lock_resource_id);
    }
    lock_res->async_unlock_invocation = request->invocation;
    reply->status = saLckResourceUnlockAsync(request->invocation, 
                                             lock_res->lock_id);
}

void 
ais_test_daemon_handle_dispatch_request(ais_test_lck_request_t *request,
                                        ais_test_lck_reply_t *reply)
{
    lock_resource_t *lock_res = NULL;

    ais_test_log("Received a dispatch request for id %d\n",
                 request->lock_resource_id);
    lock_res = lookup_lock_resource(request->lock_resource_id);
    if (NULL == lock_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->lock_resource_id);
    }
    reply->status = saLckDispatch(lock_res->lck_handle, 
                                  request->dispatch_flags);
}

void 
ais_test_daemon_handle_signal_notify_request(ais_test_lck_request_t *request,
                                             ais_test_lck_reply_t *reply)
{
    lock_resource_t *lock_res = NULL;

    ais_test_log("Received a signal notify count request for id %d\n",
                 request->lock_resource_id);
    lock_res = lookup_lock_resource(request->lock_resource_id);
    if (NULL == lock_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->lock_resource_id);
    }
    reply->last_delivered_waiter_signal = 
        lock_res->last_delivered_waiter_signal;
    reply->waiter_signal_notification_count = 
        lock_res->waiter_signal_notification_count;
    reply->status = SA_AIS_OK;
}

void 
ais_test_daemon_handle_async_lock_status_request(
    ais_test_lck_request_t *request,
    ais_test_lck_reply_t *reply)
{
    lock_resource_t *lock_res = NULL;

    ais_test_log("Received an async lock status request for id %d\n",
                 request->lock_resource_id);
    lock_res = lookup_lock_resource(request->lock_resource_id);
    if (NULL == lock_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->lock_resource_id);
    }
    reply->lock_status = lock_res->lock_status;
    reply->async_lock_invocation = lock_res->async_lock_invocation;
    reply->status = lock_res->async_lock_error_status;
}

void 
ais_test_daemon_handle_async_unlock_status_request(
    ais_test_lck_request_t *request,
    ais_test_lck_reply_t *reply)
{
    lock_resource_t *lock_res = NULL;

    ais_test_log("Received an async unlock status request for id %d\n",
                 request->lock_resource_id);
    lock_res = lookup_lock_resource(request->lock_resource_id);
    if (NULL == lock_res) {
        ais_test_abort("Unknown resource id %d\n",
                       request->lock_resource_id);
    }
    reply->lock_status = lock_res->lock_status;
    reply->async_unlock_invocation = lock_res->async_unlock_invocation;
    reply->status = lock_res->async_unlock_error_status;
}

void
ais_test_daemon_handle_incoming_client_message(int client_connection_fd,
                                               void *void_request)
{
    ais_test_lck_request_t *request = (ais_test_lck_request_t *)void_request;
    ais_test_lck_reply_t *reply;

    if (NULL == request) {
        ais_test_abort("Invalid (NULL) request\n");
    }

    reply = (ais_test_lck_reply_t *)malloc(sizeof(ais_test_lck_reply_t));
    memset(reply, 0, sizeof(ais_test_lck_reply_t));

    switch(request->op) {
        case AIS_TEST_LCK_REQUEST_CREATE_TEST_RESOURCE:
            ais_test_daemon_handle_create_test_res_request(request, reply);
            break;
        case AIS_TEST_LCK_REQUEST_LOCK_INITALIZE:
            ais_test_daemon_handle_init_request(request, reply);
            break;
        case AIS_TEST_LCK_REQUEST_LOCK_SELECTION_OBJECT_GET:
            ais_test_daemon_handle_selection_object_request(request, reply);
            break;
        case AIS_TEST_LCK_REQUEST_LOCK_RESOURCE_OPEN:
            ais_test_daemon_handle_resource_open_request(request, reply);
            break;
        case AIS_TEST_LCK_REQUEST_LOCK_RESOURCE_CLOSE:
            ais_test_daemon_handle_resource_close_request(request, reply);
            break;
        case AIS_TEST_LCK_REQUEST_LOCK_FINALIZE:
            ais_test_daemon_handle_finalize_request(request, reply);
            break;
        case AIS_TEST_LCK_REQUEST_LOCK_SYNC:
            ais_test_daemon_handle_sync_lock_request(request, reply);
            break;
        case AIS_TEST_LCK_REQUEST_LOCK_ASYNC:
            ais_test_daemon_handle_async_lock_request(request, reply);
            break;
        case AIS_TEST_LCK_REQUEST_UNLOCK_SYNC:
            ais_test_daemon_handle_sync_unlock_request(request, reply);
            break;
        case AIS_TEST_LCK_REQUEST_UNLOCK_ASYNC:
            ais_test_daemon_handle_async_unlock_request(request, reply);
            break;
        case AIS_TEST_LCK_REQUEST_DISPATCH:
            ais_test_daemon_handle_dispatch_request(request, reply);
            break;
        case AIS_TEST_LCK_REQUEST_WAITER_SIGNAL_NOTIFY_COUNT:
            ais_test_daemon_handle_signal_notify_request(request, reply);
            break;
        case AIS_TEST_LCK_REQUEST_ASYNC_LOCK_STATUS:
            ais_test_daemon_handle_async_lock_status_request(request, reply);
            break;
        case AIS_TEST_LCK_REQUEST_ASYNC_UNLOCK_STATUS:
            ais_test_daemon_handle_async_unlock_status_request(request, reply);
            break;
        default:
            ais_test_abort("Daemon received request with unknown op %d\n",
                           request->op);
    }
    free(request);

    ais_test_send_reply(client_connection_fd, reply,
                        sizeof(ais_test_lck_reply_t));
    free(reply);
}

void
ais_test_daemon_handle_incoming_lock_message(gpointer data, gpointer user_data)
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

    ais_test_log("Incoming request on lock selection fd %d\n",
                 lock_res->selection_object);
    err = saLckDispatch(lock_res->lck_handle, lock_res->dispatch_flags);
    if (SA_AIS_OK != err) {
        ais_test_log("Error %s performing saLckDispatch\n",
                     get_error_string(err));
    }
}

void ais_test_daemon_add_lock_resource_to_fdset(gpointer data, 
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

void ais_test_daemon_add_fds(
    int *max_fd,
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    fd_set_key_t set_key;

    set_key.set = read_fd_set;
    set_key.largest_fd = *max_fd;

    g_list_foreach(lock_list, ais_test_daemon_add_lock_resource_to_fdset,
                   &set_key);
    *max_fd = set_key.largest_fd;
}

void
ais_test_daemon_check_fds(
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    g_list_foreach(lock_list,
                   ais_test_daemon_handle_incoming_lock_message,
                   read_fd_set);
}

/*
int
ais_test_client_send_sync_lock_request(ais_test_lck_request_op_t op,
                                           const char *lock_name)
{
    strcpy(request->lock_name, lock_name);

*/

SaAisErrorT
ais_test_client_handle_create_test_res_request(
    int fd,
    ais_test_lck_request_t *request)
{
    ais_test_lck_reply_t *reply;
    SaAisErrorT status;
 
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_lck_request_t),
                                  sizeof(ais_test_lck_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    if (SA_AIS_OK == status) {
        ais_test_log("Resource ID=%d\n", reply->lock_resource_id);
    }
    free(reply);
    return(status);
}

SaAisErrorT
ais_test_client_handle_init_request(int fd,
                                    ais_test_lck_request_t *request,
                                    int lock_resource_id,
                                    SaVersionT *sa_version,
                                    int resource_open_cb_flag,
                                    int lock_grant_cb_flag,
                                    int lock_waiter_cb_flag,
                                    int resource_unlock_cb_flag,
                                    SaDispatchFlagsT dispatch_flags,
                                    int null_lock_handle_flag,
                                    int null_callbacks_flag,
                                    int null_version_flag)
{
    ais_test_lck_reply_t *reply;
    SaAisErrorT status;
 
    request->lock_resource_id = lock_resource_id;
    request->sa_version = *sa_version;
    request->resource_open_cb_flag = resource_open_cb_flag;
    request->lock_grant_cb_flag = lock_grant_cb_flag;
    request->lock_waiter_cb_flag = lock_waiter_cb_flag;
    request->resource_unlock_cb_flag = resource_unlock_cb_flag;
    request->dispatch_flags = dispatch_flags;
    request->null_lock_handle_flag = null_lock_handle_flag;
    request->null_callbacks_flag = null_callbacks_flag;
    request->null_version_flag = null_version_flag;
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_lck_request_t),
                                  sizeof(ais_test_lck_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }
    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_selection_object_request(int fd,
                                             ais_test_lck_request_t *request,
                                             int lock_resource_id,
                                             int null_selection_object_flag)
{
    ais_test_lck_reply_t *reply;
    SaAisErrorT status;
 
    request->lock_resource_id = lock_resource_id;
    request->null_selection_object_flag = null_selection_object_flag;
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_lck_request_t),
                                  sizeof(ais_test_lck_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_resource_open_request(int fd,
                                             ais_test_lck_request_t *request,
                                             int lock_resource_id,
                                             const char *lock_name)
{
    ais_test_lck_reply_t *reply;
    SaAisErrorT status;
 
    request->lock_resource_id = lock_resource_id;
    strcpy(request->lock_name, lock_name);
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_lck_request_t),
                                  sizeof(ais_test_lck_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_resource_close_request(int fd,
                                              ais_test_lck_request_t *request,
                                              int lock_resource_id)
{
    ais_test_lck_reply_t *reply;
    SaAisErrorT status;
 
    request->lock_resource_id = lock_resource_id;
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_lck_request_t),
                                  sizeof(ais_test_lck_reply_t));
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
    ais_test_lck_request_t *request, 
    int lock_resource_id)
{
    ais_test_lck_reply_t *reply;
    SaAisErrorT status;
 
    request->lock_resource_id = lock_resource_id;
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_lck_request_t),
                                  sizeof(ais_test_lck_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_sync_lock_request(int fd,
                                         ais_test_lck_request_t *request,
                                         int lock_resource_id,
                                         SaLckLockModeT lock_mode,
                                         SaLckLockFlagsT lock_flags,
                                         SaLckLockStatusT expected_status,
                                         SaLckWaiterSignalT waiter_signal,
                                         int null_lock_id_flag,
                                         int null_lock_status_flag)
{
    ais_test_lck_reply_t *reply;
    SaAisErrorT status;
 
    request->lock_resource_id = lock_resource_id;
    request->lock_mode = lock_mode;
    request->lock_flags = lock_flags;
    request->expected_lock_status = expected_status;
    request->null_lock_id_flag = null_lock_id_flag;
    request->null_lock_status_flag = null_lock_status_flag;
    request->waiter_signal = waiter_signal;
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_lck_request_t),
                                  sizeof(ais_test_lck_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_async_lock_request(int fd,
                                          ais_test_lck_request_t *request,
                                          int lock_resource_id,
                                          SaLckLockModeT lock_mode,
                                          SaLckLockFlagsT lock_flags,
                                          SaInvocationT invocation,
                                          SaLckWaiterSignalT waiter_signal,
                                          int null_lock_id_flag)
{
    ais_test_lck_reply_t *reply;
    SaAisErrorT status;
 
    request->lock_resource_id = lock_resource_id;
    request->lock_mode = lock_mode;
    request->lock_flags = lock_flags;
    request->invocation = invocation;
    request->waiter_signal = waiter_signal;
    request->null_lock_id_flag = null_lock_id_flag;
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_lck_request_t),
                                  sizeof(ais_test_lck_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_sync_unlock_request(int fd,
                                           ais_test_lck_request_t *request,
                                           int lock_resource_id)
{
    ais_test_lck_reply_t *reply;
    SaAisErrorT status;
 
    request->lock_resource_id = lock_resource_id;
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_lck_request_t),
                                  sizeof(ais_test_lck_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_async_unlock_request(int fd,
                                            ais_test_lck_request_t *request,
                                            int lock_resource_id,
                                            SaInvocationT invocation)
{
    ais_test_lck_reply_t *reply;
    SaAisErrorT status;
 
    request->lock_resource_id = lock_resource_id;
    request->invocation = invocation;
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_lck_request_t),
                                  sizeof(ais_test_lck_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }

    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_dispatch_request(int fd,
                                        ais_test_lck_request_t *request,
                                        int lock_resource_id,
                                        SaDispatchFlagsT dispatch_flags)
{
    ais_test_lck_reply_t *reply;
    SaAisErrorT status;
 
    request->lock_resource_id = lock_resource_id;
    request->dispatch_flags = dispatch_flags;
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_lck_request_t),
                                  sizeof(ais_test_lck_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }
    status = reply->status;
    free(reply);
    return(status);
}

int
ais_test_client_handle_notify_count_request(int fd,
                                            ais_test_lck_request_t *request,
                                            int lock_resource_id)
{
    ais_test_lck_reply_t *reply;
    SaAisErrorT status;
 
    request->lock_resource_id = lock_resource_id;
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_lck_request_t),
                                  sizeof(ais_test_lck_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }
    status = reply->status;
    if (SA_AIS_OK == status) {
        ais_test_log("Last Delivered Lock Waiter Signal=%lld\n", 
                     reply->last_delivered_waiter_signal);
        ais_test_log("Lock Waiter Notification Count=%d\n", 
                     reply->waiter_signal_notification_count);
    }
    free(reply);
    return(status);
}

int
ais_test_client_handle_async_lock_status_request(
    int fd,
    ais_test_lck_request_t *request,
    int lock_resource_id)
{
    ais_test_lck_reply_t *reply;
    SaAisErrorT status;
 
    request->lock_resource_id = lock_resource_id;
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_lck_request_t),
                                  sizeof(ais_test_lck_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }
    status = reply->status;
    ais_test_log("Async Lock Invocation=%d\n", 
                 reply->async_lock_invocation);
    ais_test_log("Async Lock Status=%s\n", 
                 ais_test_map_lock_status_to_string(reply->lock_status));
    free(reply);
    return(status);
}

int
ais_test_client_handle_async_unlock_status_request(
    int fd,
    ais_test_lck_request_t *request,
    int lock_resource_id)
{
    ais_test_lck_reply_t *reply;
    SaAisErrorT status;
 
    request->lock_resource_id = lock_resource_id;
    reply = ais_test_send_request(fd, request,
                                  sizeof(ais_test_lck_request_t),
                                  sizeof(ais_test_lck_reply_t));
    if (NULL == reply) {
        ais_test_abort("Received no reply from the daemon\n");
    }
    status = reply->status;
    ais_test_log("Async Unlock Invocation=%d\n", 
                 reply->async_unlock_invocation);
    ais_test_log("Async Unlock Status=%s\n", 
                 ais_test_map_lock_status_to_string(reply->lock_status));
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
#define LOCK_NAME_OPTION 9
#define TIMEOUT_OPTION 10 
#define SET_RESOURCE_OPEN_CB_OPTION 11
#define SET_LOCK_GRANT_CB_OPTION 12
#define SET_LOCK_WAITER_CB_OPTION 13
#define SET_RESOURCE_UNLOCK_CB_OPTION 14
#define LOCK_RESOURCE_ID_OPTION 15
#define LOCK_MODE_OPTION 16
#define WAITER_SIGNAL_OPTION 17
#define VERBOSE_OPTION 18
#define EXPECTED_LOCK_STATUS_OPTION 19
#define DISPATCH_FLAGS_OPTION 20 
#define LOCK_FLAG_NO_QUEUE_OPTION 21
#define LOCK_FLAG_ORPHAN_OPTION 22
#define LOCK_FLAG_INVALID_OPTION 23
#define NULL_LOCK_HANDLE_OPTION 24
#define NULL_CALLBACKS_OPTION 25
#define NULL_VERSION_OPTION 26
#define NULL_LOCK_ID_OPTION 27
#define NULL_LOCK_STATUS_OPTION 28
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
    int     	        lock_name_flag = 0;
    int                 timeout_flag = 0;
    int                 resource_open_cb_flag = 0;
    int                 lock_grant_cb_flag = 0;
    int                 lock_waiter_cb_flag = 0;
    int                 resource_unlock_cb_flag = 0;
    int                 lock_resource_id_flag = 0;
    int     	        lock_mode_flag = 0;
    int     	        waiter_signal_flag = 0;
    int     	        verbose_flag = 0;
    int     	        expected_lock_status_flag = 0;
    int     	        dispatch_type_flag = 0;
    int     	        null_lock_handle_flag = 0;
    int     	        null_callbacks_flag = 0;
    int     	        null_version_flag = 0;
    int     	        null_lock_id_flag = 0;
    int     	        null_lock_status_flag = 0;
    int     	        null_selection_object_flag = 0;
    int                 lock_flags_no_queue_flag = 0;
    int                 lock_flags_orphan_flag = 0;
    int                 lock_flags_invalid_flag = 0;
    int                 invocation_flag = 0;
    int                 version_release_code_flag = 0;
    int                 version_major_flag = 0;
    int                 version_minor_flag = 0;
    char                run_path[BUF_SIZE];
    char                pid_file[BUF_SIZE];
    char                log_file[BUF_SIZE];
    char                socket_file[BUF_SIZE];
    char             	op_name[BUF_SIZE];
    char             	lock_name[SA_MAX_NAME_LENGTH+1];
    ais_test_lck_request_op_t op;
    int                 timeout = 0;
    int                 lock_resource_id = 0;
    SaVersionT           sa_version;
    SaLckLockModeT      lock_mode = 0;
    SaLckWaiterSignalT  waiter_signal = 0;
    SaLckLockStatusT    expected_lock_status = 0;
    SaLckLockFlagsT     lock_flags = 0;
    SaDispatchFlagsT    dispatch_flags = 0;
    SaInvocationT       invocation = 0;
    int next_option = 0;
    ais_test_lck_request_t *request;
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
        { "set-resource-open-cb", 0, NULL, SET_RESOURCE_OPEN_CB_OPTION},
        { "set-lock-grant-cb", 0, NULL, SET_LOCK_GRANT_CB_OPTION},
        { "set-lock-waiter-cb", 0, NULL, SET_LOCK_WAITER_CB_OPTION},
        { "set-resource-unlock-cb", 0, NULL, SET_RESOURCE_UNLOCK_CB_OPTION},
        { "resource-id", 1, NULL, LOCK_RESOURCE_ID_OPTION},
        { "lock-name", 1, NULL, LOCK_NAME_OPTION},
        { "lock-mode", 1, NULL, LOCK_MODE_OPTION},
        { "invocation", 1, NULL, INVOCATION_OPTION},
        { "timeout", 1, NULL, TIMEOUT_OPTION},
        { "waiter-signal", 1, NULL, WAITER_SIGNAL_OPTION},
        { "verbose", 0, NULL, VERBOSE_OPTION},
        { "expected-status", 1, NULL, EXPECTED_LOCK_STATUS_OPTION},
        { "dispatch-flags", 1, NULL, DISPATCH_FLAGS_OPTION},
        { "lock-flag-no-queue", 0, NULL, LOCK_FLAG_NO_QUEUE_OPTION},
        { "lock-flag-orphan", 0, NULL, LOCK_FLAG_ORPHAN_OPTION},
        { "lock-flag-invalid", 0, NULL, LOCK_FLAG_INVALID_OPTION},
        { "null-lck-handle", 0, NULL, NULL_LOCK_HANDLE_OPTION},
        { "null-callbacks", 0, NULL, NULL_CALLBACKS_OPTION},
        { "null-version", 0, NULL, NULL_VERSION_OPTION},
        { "null-lock-id", 0, NULL, NULL_LOCK_ID_OPTION},
        { "null-lock-status", 0, NULL, NULL_LOCK_STATUS_OPTION},
        { "null-selection-object", 0, NULL, NULL_SELECTION_OBJECT_OPTION},
        { "version-release-code", 1, NULL, VERSION_RELEASE_CODE_OPTION},
        { "version-major-code", 1, NULL, VERSION_MAJOR_OPTION},
        { "version-minor-code", 1, NULL, VERSION_MINOR_OPTION},
        { NULL,       0, NULL, 0   }   /* Required at end of array.  */
    };

    memset(lock_name, 0, sizeof(lock_name));
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
            case LOCK_NAME_OPTION:
                if (lock_name_flag) {
                    usage();
                }
                lock_name_flag++;
                strcpy(lock_name, optarg);
                break;
            case TIMEOUT_OPTION:
                if (timeout_flag) {
                    usage();
                }
                timeout_flag++;
                timeout = atoi(optarg);
                break;
            case SET_RESOURCE_OPEN_CB_OPTION:
                if (resource_open_cb_flag) {
                    usage();
                }
                resource_open_cb_flag++;
                break;
            case SET_LOCK_GRANT_CB_OPTION:
                if (lock_grant_cb_flag) {
                    usage();
                }
                lock_grant_cb_flag++;
                break;
            case SET_LOCK_WAITER_CB_OPTION:
                if (lock_waiter_cb_flag) {
                    usage();
                }
                lock_waiter_cb_flag++;
                break;
            case SET_RESOURCE_UNLOCK_CB_OPTION:
                if (resource_unlock_cb_flag) {
                    usage();
                }
                resource_unlock_cb_flag++;
                break;
            case LOCK_RESOURCE_ID_OPTION:
                if (lock_resource_id_flag) {
                    usage();
                }
                lock_resource_id_flag++;
                lock_resource_id = atoi(optarg);
                break;
            case LOCK_MODE_OPTION:
                if (lock_mode_flag) {
                    usage();
                }
                lock_mode_flag++;
                if (0 == strcmp(optarg, "PR")) {
                    lock_mode = SA_LCK_PR_LOCK_MODE;
                } else if (0 == strcmp(optarg, "EX")) {
                    lock_mode = SA_LCK_EX_LOCK_MODE;
                } else if (0 == strcmp(optarg, "INVALID")) {
                    lock_mode = -1;
                } else {
                    usage();
                }
                break;
            case WAITER_SIGNAL_OPTION:
                if (waiter_signal_flag) {
                    usage();
                }
                waiter_signal_flag++;
                waiter_signal = atoi(optarg);
                break;
            case VERBOSE_OPTION:
                if (verbose_flag) {
                    usage();
                }
                verbose_flag++;
                break;
            case EXPECTED_LOCK_STATUS_OPTION:
                if (expected_lock_status_flag) {
                    usage();
                }
                expected_lock_status_flag++;
                if (0 == strcmp(optarg, "SA_LCK_LOCK_GRANTED")) {
                    expected_lock_status = SA_LCK_LOCK_GRANTED;
                } else if (0 == strcmp(optarg, "SA_LCK_LOCK_DEADLOCK")) {
                    expected_lock_status = SA_LCK_LOCK_DEADLOCK;
                } else if (0 == strcmp(optarg, "SA_LCK_LOCK_NOT_QUEUED")) {
                    expected_lock_status = SA_LCK_LOCK_NOT_QUEUED;
                } else if (0 == strcmp(optarg, "SA_LCK_LOCK_TIMED_OUT")) {
                    expected_lock_status = SA_LCK_LOCK_TIMED_OUT;
                } else if (0 == strcmp(optarg, "SA_LCK_LOCK_ORPHANED")) {
                    expected_lock_status = SA_LCK_LOCK_ORPHANED;
                } else if (0 == strcmp(optarg, "SA_LCK_LOCK_NO_MORE")) {
                    expected_lock_status = SA_LCK_LOCK_NO_MORE;
                } else if (0 == strcmp(optarg, "SA_LCK_LOCK_DUPLICATE_EX")) {
                    expected_lock_status = SA_LCK_LOCK_DUPLICATE_EX;
                } else {
                    usage();
                }
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
            case LOCK_FLAG_NO_QUEUE_OPTION:
                if (lock_flags_no_queue_flag) {
                    usage();
                }
                lock_flags_no_queue_flag++;
                lock_flags |= SA_LCK_LOCK_NO_QUEUE;
                break;
            case LOCK_FLAG_ORPHAN_OPTION:
                if (lock_flags_orphan_flag) {
                    usage();
                }
                lock_flags_orphan_flag++;
                lock_flags |= SA_LCK_LOCK_ORPHAN;
                break;
            case LOCK_FLAG_INVALID_OPTION:
                if (lock_flags_invalid_flag) {
                    usage();
                }
                lock_flags_invalid_flag++;
                lock_flags = -1;
                break;
            case NULL_LOCK_HANDLE_OPTION:
                if (null_lock_handle_flag) {
                    usage();
                }
                null_lock_handle_flag++;
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
            case NULL_LOCK_ID_OPTION:
                if (null_lock_id_flag) {
                    usage();
                }
                null_lock_id_flag++;
                break;
            case NULL_LOCK_STATUS_OPTION:
                if (null_lock_status_flag) {
                    usage();
                }
                null_lock_status_flag++;
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

    request = (ais_test_lck_request_t *)
              malloc(sizeof(ais_test_lck_request_t));
    memset(request, 0, sizeof(ais_test_lck_request_t));
    request->op = op;
    request->requestor_pid = getpid();
    switch(op) {
        case AIS_TEST_LCK_REQUEST_CREATE_TEST_RESOURCE:
            status = 
                ais_test_client_handle_create_test_res_request(
                    client_fd, request);
            break;
        case AIS_TEST_LCK_REQUEST_LOCK_INITALIZE:
            if (!lock_resource_id_flag || !dispatch_type_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_init_request(client_fd, 
                                                    request,
                                                    lock_resource_id,
                                                    &sa_version,
                                                    resource_open_cb_flag,
                                                    lock_grant_cb_flag,
                                                    lock_waiter_cb_flag,
                                                    resource_unlock_cb_flag,
                                                    dispatch_flags,
                                                    null_lock_handle_flag,
                                                    null_callbacks_flag,
                                                    null_version_flag);
            break;
        case AIS_TEST_LCK_REQUEST_LOCK_SELECTION_OBJECT_GET:
            if (!lock_resource_id_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_selection_object_request(
                    client_fd, request, lock_resource_id,
                    null_selection_object_flag);
            break;
        case AIS_TEST_LCK_REQUEST_LOCK_RESOURCE_OPEN:
            if (!lock_resource_id_flag || !lock_name_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_resource_open_request(
                    client_fd, request, lock_resource_id, lock_name);
            break;
        case AIS_TEST_LCK_REQUEST_LOCK_RESOURCE_CLOSE:
            if (!lock_resource_id_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_resource_close_request(
                    client_fd, request, lock_resource_id);
            break;
        case AIS_TEST_LCK_REQUEST_LOCK_FINALIZE:
            if (!lock_resource_id_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_resource_finalize_request(
                    client_fd, request, lock_resource_id);
            break;
        case AIS_TEST_LCK_REQUEST_LOCK_SYNC:
            if (!lock_resource_id_flag || !lock_mode_flag ||
                !expected_lock_status_flag || !waiter_signal_flag) {
                usage();
            }
            status = ais_test_client_handle_sync_lock_request(
                client_fd, request, lock_resource_id, lock_mode,
                lock_flags, expected_lock_status, waiter_signal,
                null_lock_id_flag, null_lock_status_flag);
            break;
        case AIS_TEST_LCK_REQUEST_LOCK_ASYNC:
            if (!lock_resource_id_flag || !lock_mode_flag ||
                !invocation_flag || !waiter_signal_flag) {
                usage();
            }
            status = ais_test_client_handle_async_lock_request(
                client_fd, request, lock_resource_id, lock_mode,
                lock_flags, invocation, waiter_signal, null_lock_id_flag);
            break;
        case AIS_TEST_LCK_REQUEST_UNLOCK_SYNC:
            if (!lock_resource_id_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_sync_unlock_request(
                    client_fd, request, lock_resource_id);
            break;
        case AIS_TEST_LCK_REQUEST_UNLOCK_ASYNC:
            if (!lock_resource_id_flag || !invocation_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_async_unlock_request(
                    client_fd, request, lock_resource_id,
                    invocation);
            break;
        case AIS_TEST_LCK_REQUEST_DISPATCH:
            /*
             * Can specify either a specific handle or a lock_resource_id
             */
            if (!lock_resource_id_flag || !dispatch_type_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_dispatch_request(
                    client_fd, request, lock_resource_id, dispatch_flags);
            break;
        case AIS_TEST_LCK_REQUEST_WAITER_SIGNAL_NOTIFY_COUNT:
            if (!lock_resource_id_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_notify_count_request(
                    client_fd, request, lock_resource_id);
            break;
        case AIS_TEST_LCK_REQUEST_ASYNC_LOCK_STATUS:
            if (!lock_resource_id_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_async_lock_status_request(
                    client_fd, request, lock_resource_id);
            break;
        case AIS_TEST_LCK_REQUEST_ASYNC_UNLOCK_STATUS:
            if (!lock_resource_id_flag) {
                usage();
            }
            status = 
                ais_test_client_handle_async_unlock_status_request(
                    client_fd, request, lock_resource_id);
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
