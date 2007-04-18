/**********************************************************************
 *
 *	I N C L U D E S
 *
 **********************************************************************/
#include "saftest_driver_lib_utils.h"
#include "saftest_driver.h"
#include "saftest_list.h"
#include "saftest_log.h"
#include "saftest_comm.h"
#include "string.h"

/*
 * one resource per connection client
 */
typedef struct client_resource {
    int client_connection_fd;
} client_resource_t;

typedef struct driver_thread {
    int main_thread;
    pthread_t thread_id;
    char socket_file[SAFTEST_STRING_LENGTH + 1];
    int listen_fd;
    saftest_list client_list;
} driver_thread_t;

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
static int   daemon_initialized = 0;
static int   daemon_is_long_lived = 0;
static char  daemon_pid_file[BUF_SIZE];
static pthread_t main_thread_id;
saftest_list shlib_list = NULL;
saftest_list thread_list = NULL;

void
saftest_daemon_handle_message(saftest_map_table_entry_t *first_entry,
                              int client_connection_fd,
                              saftest_msg_t *request);

const char *get_library_id()
{
    return("MAIN");
}

shared_library_t *
lookup_shared_library(const char *library_id)
{
    saftest_list_element element;
    shared_library_t *shlib;

    for (element = saftest_list_first(shlib_list);
         NULL != element;
         element = saftest_list_next(element)) {
        shlib = (shared_library_t *)element->data;
        if (0 == strcmp(shlib->library_id, library_id)) {
            return(shlib);
        }
    }
    return(NULL);
}

client_resource_t *
add_client_resource(driver_thread_t *dt)
{
    client_resource_t *res;

    res = malloc(sizeof(client_resource_t));
    assert(NULL != res);
    memset(res, 0, sizeof(client_resource_t));

    saftest_list_element_create(dt->client_list, res);
    return(res);
}

void
delete_client_resource(client_resource_t *res)
{
    driver_thread_t *dt;
    saftest_list_element thread_element;
    saftest_list_element client_element;

    for (thread_element = saftest_list_first(thread_list);
         NULL != thread_element;
         thread_element = saftest_list_next(thread_element)) {
        dt = (driver_thread_t *)thread_element->data;
        for (client_element = saftest_list_first(dt->client_list);
             NULL != client_element;
             client_element = saftest_list_next(client_element)) {
            if (client_element->data == res) {
                saftest_list_element_delete_deep(&client_element, free);
                break;
            }
        }
    }
}

client_resource_t *
lookup_client_resource(driver_thread_t *dt, int fd)
{
    saftest_list_element element;
    client_resource_t *res;

    for (element = saftest_list_first(dt->client_list);
         NULL != element;
         element = saftest_list_next(element)) {
        res = (client_resource_t *)element->data;
        if (res->client_connection_fd == fd) {
            return(res);
        }
    }
    return(NULL);
}

static driver_thread_t *
add_driver_thread()
{
    driver_thread_t *dt;

    dt = malloc(sizeof(driver_thread_t));
    assert(NULL != dt);
    memset(dt, 0, sizeof(driver_thread_t));

    dt->listen_fd = -1;
    dt->client_list = saftest_list_create();

    saftest_list_element_create(thread_list, dt);
    saftest_log("Added a new driver thread\n");
    return(dt);
}

driver_thread_t *
get_current_driver_thread()
{
    saftest_list_element element;
    driver_thread_t *dt;

    for (element = saftest_list_first(thread_list);
         NULL != element;
         element = saftest_list_next(element)) {
        dt = (driver_thread_t *)element->data;
        if (dt->thread_id == pthread_self()) {
            return(dt);
        }
    }
    saftest_abort("We have to have an object representing ourselves "
                  "(self_id = %ld)\n", pthread_self());
    return(NULL);
}

void
shlib_add_fds(void *data, void *key)
{
    shared_library_t *shlib = data;
    fd_set_key_t *set_key = (fd_set_key_t *)key;

    if (NULL == data) {
        return;
    }

    shlib->daemon_add_fds_func(&(set_key->largest_fd), set_key->set,
                               NULL, NULL);
}

void
shlib_check_fds(void *data, void *key)
{
    shared_library_t *shlib = data;
    fd_set *fd_mask = (fd_set *)key;

    if (NULL == data) {
        return;
    }

    shlib->daemon_check_fds_func(fd_mask, NULL, NULL);
}

static void
check_fds(
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    saftest_list_each(shlib_list, shlib_check_fds, read_fd_set);
}

static void
fillout_read_fdset(
    int *max_fd,
    fd_set *read_fd_set)

{
    fd_set_key_t set_key;

    FD_ZERO(read_fd_set);

    *max_fd = -1;

    set_key.set = read_fd_set;
    set_key.largest_fd = *max_fd;

    saftest_list_each(shlib_list, shlib_add_fds, &set_key);
    *max_fd = set_key.largest_fd;
}

void
handle_accept(driver_thread_t *dt)
{
    client_resource_t *res = NULL;

    res = add_client_resource(dt);

    saftest_uds_accept(dt->listen_fd, &(res->client_connection_fd));
/*
    saftest_log("Accepted Client Connection on FD %d\n",
                res->client_connection_fd);
*/
}

static void *
select_loop(void *arg)
{
    driver_thread_t *dt = (driver_thread_t *)arg;
    fd_set read_fd_set;
    int max_fd;
    int ret;
    saftest_list_element element;
    shared_library_t *shlib;

    for (element = saftest_list_first(shlib_list);
         NULL != element;
         element = saftest_list_next(element)) {
        shlib = (shared_library_t *)element->data;
        saftest_log("Calling thread init for lib %s\n", shlib->library_id);
        shlib->daemon_thread_init_func(dt->main_thread);
    }

    saftest_uds_listen(&dt->listen_fd, dt->socket_file);
    saftest_log("Begin select loop\n");

    while (1) {
        fillout_read_fdset(&max_fd, &read_fd_set);

        unblock_signals();
        ret = select(max_fd+1, &read_fd_set, NULL, NULL, NULL);
        block_signals();

        if (-1 == ret) {
            saftest_log("Error %d (%s) from select()\n",
                         errno, strerror(errno));
            clear_pid_file(daemon_pid_file);
            err_exit("Exiting from select error\n");
        }
        if (0 != ret) {
            check_fds(&read_fd_set, NULL, NULL);
        } else {
            /* This is where we would put a timeout handler if we had one */
        }
    }
    return(NULL);
}

static saftest_map_table_entry_t *
saftest_get_map_table_entry(saftest_map_table_entry_t *first_entry,
                            const char *request_op)
{
    int ndx = 0;

    for (ndx = 0; NULL != first_entry[ndx].request_op; ndx++) {
        if (0 == strcmp(first_entry[ndx].request_op, request_op)) {
            return &(first_entry[ndx]);
        }
    }
    return(NULL);
}

void
saftest_daemon_handle_message(saftest_map_table_entry_t *first_entry,
                              int client_connection_fd,
                              saftest_msg_t *request)
{
    saftest_msg_t *reply = NULL;
    saftest_map_table_entry_t *entry;

    if (NULL == request) {
        saftest_abort("Invalid (NULL) request\n");
    }

    entry = saftest_get_map_table_entry(first_entry,
                                        saftest_msg_get_msg_type(request));
    entry->daemon_handler(entry, request, &reply);

    if (NULL != reply) {
        saftest_send_reply(client_connection_fd, reply);
    }
}

void
saftest_daemon_handle_incoming_client_message(
    void *data, void *key)
{
    saftest_msg_t *request;
    client_resource_t *res = data;
    fd_set *fd_mask = (fd_set *)key;
    shared_library_t *shlib = NULL;

    if (NULL == data) {
        return;
    }

    if (!FD_ISSET(res->client_connection_fd, fd_mask)) {
        return;
    }

    request = saftest_recv_request(res->client_connection_fd);
    if (NULL == request) {
        /*
        saftest_log("Failed to recv request from client on fd %d."
                     "Closing connection.\n",
                     res->client_connection_fd);
        */
        close(res->client_connection_fd);
        delete_client_resource(res);
        return;
    }

    /*
    saftest_log("\nlookup_shared_library request is %d",
               request);
    */
    shlib = lookup_shared_library(
               saftest_msg_get_destination_library_id(request));
    assert(NULL!= shlib);
    saftest_daemon_handle_message(shlib->get_map_table_func(),
                                  res->client_connection_fd,
                                  request);
}

void saftest_daemon_add_client_to_fdset(void *data, void *key)
{
    client_resource_t *res;
    fd_set_key_t *set_key = (fd_set_key_t *)key;

    if (NULL == data) {
        return;
    }

    res = (client_resource_t *)data;

    FD_SET(res->client_connection_fd, set_key->set);
    if (res->client_connection_fd > set_key->largest_fd) {
        set_key->largest_fd = res->client_connection_fd;
    }

    return;
}

/* Begin per-shlib functions */
void saftest_daemon_init(FILE *log_fp)
{
    assert(NULL != log_fp);
    saftest_log_set_fp(log_fp);
}

void saftest_daemon_thread_init(FILE *log_fp)
{
}

void saftest_daemon_add_fds(
    int *max_fd,
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    driver_thread_t *dt = NULL;
    fd_set_key_t set_key;

    dt = get_current_driver_thread();

    FD_SET(dt->listen_fd, read_fd_set);
    if (dt->listen_fd > *max_fd) {
        *max_fd = dt->listen_fd;
    }

    set_key.set = read_fd_set;
    set_key.largest_fd = *max_fd;

    saftest_list_each(dt->client_list, saftest_daemon_add_client_to_fdset,
                      &set_key);
    *max_fd = set_key.largest_fd;
}

void
saftest_daemon_check_fds(
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    driver_thread_t *dt = NULL;

    dt = get_current_driver_thread();

    if (FD_ISSET(dt->listen_fd, read_fd_set)) {
        handle_accept(dt);
    } else {
        saftest_list_each(dt->client_list,
                          saftest_daemon_handle_incoming_client_message,
                          read_fd_set);
    }
}

void saftest_daemon_main_lib_main(
    saftest_list shlib_list_ptr,
    const char *main_socket_file_str,
    const char *daemon_pid_file_str)
{
    driver_thread_t *dt = NULL;
    shlib_list = shlib_list_ptr;
    strcpy(daemon_pid_file, daemon_pid_file_str);

    /* Setup an object to represent the main thread */
    thread_list = saftest_list_create();
    dt = add_driver_thread();
    dt->main_thread = TRUE;
    main_thread_id = dt->thread_id = pthread_self();
    strcpy(dt->socket_file, main_socket_file_str);

    /* Enter the select loop */
    select_loop(dt);
}

/* These are the message handlers for the "MAIN" (base class) destination. */

void
saftest_daemon_handle_driver_initialize_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    saftest_assert(0 == daemon_initialized,
                   "Initialize message can only be sent once\n");
    if (0 == strcmp(saftest_msg_get_str_value(request,
                                              "SAFTEST_DRIVER_LONG_LIVED"),
                    "TRUE")) {
        daemon_is_long_lived = 1;
    } else if (0 ==
               strcmp(saftest_msg_get_str_value(request,
                                                "SAFTEST_DRIVER_LONG_LIVED"),
              "FALSE")) {
        daemon_is_long_lived = 0;
    } else {
        saftest_abort("Unknown long-lived value \"%s\"\n",
                      saftest_msg_get_str_value(request,
                                                "SAFTEST_DRIVER_LONG_LIVED"));
    }
    saftest_log("Received an initialize request with driver type %s-lived\n",
                daemon_is_long_lived ? "long" : "short");
    daemon_initialized = 1;
             
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 0);
}  

void
saftest_daemon_handle_driver_status_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    saftest_log("Received a status request\n");
    saftest_assert(0 != daemon_initialized,
                   "Daemon must be initialized first\n");
    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 0);
}

void
saftest_daemon_handle_driver_create_thread_request(
    saftest_map_table_entry_t *map_entry,
    saftest_msg_t *request,
    saftest_msg_t **reply)
{
    driver_thread_t *dt;
    int err = 0;

    saftest_log("Received a create thread request\n");
    saftest_assert(0 != daemon_initialized,
                   "Daemon must be initialized first\n");
    saftest_assert(main_thread_id == pthread_self(),
                   "You can only create new threads from the main thread\n");

    dt = add_driver_thread();
    strcpy(dt->socket_file, 
           saftest_msg_get_str_value(request, "THREAD_SOCKET_FILE"));

    err = pthread_create(&dt->thread_id, NULL,
                         select_loop,
                         (void*)dt);
    saftest_assert(0 == err, "Error %s creating new thread\n",
                   strerror(err));
    saftest_log("Added a new thread with id %ld\n", dt->thread_id);

    (*reply) = saftest_reply_msg_create(request, map_entry->reply_op, 0);
}

SAFTEST_MAP_TABLE_BEGIN(MAIN)

SAFTEST_MAP_TABLE_ENTRY(
    "DRIVER_INITIALIZE_REQ", "DRIVER_INITIALIZE_REPLY",
    saftest_client_generic_handle_request,
    saftest_daemon_handle_driver_initialize_request)
SAFTEST_MAP_TABLE_ENTRY(
    "DRIVER_STATUS_REQ", "DRIVER_STATUS_REPLY",
    saftest_client_generic_handle_request,
    saftest_daemon_handle_driver_status_request)
SAFTEST_MAP_TABLE_ENTRY(
    "DRIVER_CREATE_THREAD_REQ", "DRIVER_CREATE_THREAD_REPLY",
    saftest_client_generic_handle_request,
    saftest_daemon_handle_driver_create_thread_request)

SAFTEST_MAP_TABLE_END(MAIN)
