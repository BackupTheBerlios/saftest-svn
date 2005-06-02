#include "saftest_driver.h"

typedef struct shared_library {
    void *handle;
    char path[BUF_SIZE+1];
    char library_id[BUF_SIZE+1];
    int library_message_size;
    int loaded;
    saftest_driver_handle_client_message_func_t daemon_handle_message_func;
    saftest_driver_add_fds_func_t daemon_add_fds_func;
    saftest_driver_check_fds_func_t daemon_check_fds_func;
    saftest_driver_client_main_func_t client_main;
} shared_library_t;

/*
 * one resource per connection client
 */
typedef struct client_resource {
    int client_connection_fd;
    char library_id[BUF_SIZE+1];
} client_resource_t;

typedef enum ais_test_request_op {
    AIS_TEST_REQUEST_INVALID=0,
    AIS_TEST_REQUEST_INIT,
} ais_test_request_op_t;

typedef struct ais_test_request {
    ais_test_request_op_t op;
    char library_id[BUF_SIZE];
} ais_test_request_t;

GList *shlib_list = NULL;
GList *client_list = NULL;
int   daemon_listen_fd = -1;
char  daemon_pid_file[BUF_SIZE];

/*
 * Each utility daemon must define this function in their own file.  Their
 * definition of it must call saftest_driver_init().
 */
extern int
saftest_driver_main(int argc, char *argv[], char *envp[]);

void
usage()
{
    printf("Server Usage: saf_driver --daemon\n");
    printf("                         --socket-file <socket path>\n");
    printf("                         --run-dir <run path>\n");
    printf("                         --log-file <log file>\n");
    printf("                         --pid-file <pid file>\n");
    printf("                         --load-libs <lib1>,<lib2>,...,<libn>\n");
    printf("                        [-no-daemonize]\n");

    printf("\n");
    exit(255);
}

shared_library_t *
add_shared_library()
{
    shared_library_t *shlib;

    shlib = malloc(sizeof(shared_library_t));
    assert(NULL != shlib);
    memset(shlib, 0, sizeof(shared_library_t));

    shlib_list = g_list_append(shlib_list, shlib);
    return(shlib);

}

shared_library_t *
lookup_shared_library(const char *library_id)
{
    GList *element;
    shared_library_t *shlib;

    for (element = g_list_first(shlib_list);
         NULL != element;
         element = g_list_next(element)) {
        shlib = (shared_library_t *)element->data;
        if (0 == strcmp(shlib->library_id, library_id)) {
            return(shlib);
        }
    }
    return(NULL);
}

void
load_shared_library(gpointer data, gpointer user_data)
{
    shared_library_t *s;
    int           fd;
    Elf32_Ehdr    elfhdr;
    struct stat   stat_buf;
    char          *error_str;
    int           load_opt;
    const char * (*get_library_id_fn)(void);
    int          (*get_library_message_size_fn)(void);
    char *error;

    if (NULL == data) {
        return;
    }
    s = (shared_library_t *)data;

    fd = open(s->path, O_RDONLY);
    if (fd == -1) {
        err_exit("Unable to open(2) %s: %s\n", s->path, strerror(errno));
    }

    if (fstat(fd, &stat_buf) != 0) {
        err_exit("Failed to stat %s: %s\n", s->path, strerror(errno));
    }

    if ((stat_buf.st_mode & S_IFREG) == 0) {
        err_exit("st_mode S_IFREG not set for %s\n", s->path);
    }

    if (stat_buf.st_size == 0) {
        err_exit("Zero length shared library file %s\n", s->path);
    }

    (void)memset(&elfhdr, 0, sizeof(elfhdr));
    if (read(fd, &elfhdr, sizeof(elfhdr)) <= 0) {
        err_exit("%s not a shared library\n", s->path);
    }
    close(fd);

    if (memcmp(elfhdr.e_ident, ELFMAG, SELFMAG) != 0) {
        /* Not a shared library */
        err_exit("%s not a shared library\n", s->path);
    } else if (elfhdr.e_type != ET_DYN) {
        err_exit("%s not a shared library\n", s->path);
    }

    load_opt = RTLD_LAZY;
    /*load_opt = RTLD_NOW;*/

    s->handle = dlopen(s->path, load_opt);
    error_str = dlerror();

    if (s->handle == NULL) {
        err_exit("Failed to load shared library %s: %s\n", s->path, error_str);
    }

    s->loaded = 1;

    dlerror(); /* Clear existing errors */
    get_library_id_fn = dlsym(s->handle, "get_library_id");    
    if ((error = dlerror()) != NULL)  {
        err_exit ("Failed to lookup get_library_id symbol: %s\n", error);
    }
    strcpy(s->library_id, get_library_id_fn());
/*
    ais_test_log("Shared library \"%s\" is for Library ID %s\n", 
                 s->path, s->library_id);
*/

    dlerror(); /* Clear existing errors */
    get_library_message_size_fn = dlsym(s->handle, "get_library_message_size");
    if ((error = dlerror()) != NULL)  {
        err_exit ("Failed to lookup get_library_message_size symbol: %s\n", 
                  error);
    }
    s->library_message_size = get_library_message_size_fn();
/*
    ais_test_log("Shared library \"%s\" has message size %d\n", 
                 s->path, s->library_message_size);
*/

    dlerror(); /* Clear existing errors */
    s->daemon_handle_message_func = 
        dlsym(s->handle, "ais_test_daemon_handle_incoming_client_message");
    if ((error = dlerror()) != NULL)  {
        err_exit ("Failed to lookup handle_message symbol: %s\n", 
                  error);
    }

    dlerror(); /* Clear existing errors */
    s->daemon_add_fds_func = dlsym(s->handle, "ais_test_daemon_add_fds");
    if ((error = dlerror()) != NULL)  {
        err_exit ("Failed to lookup ais_test_daemon_add_fds symbol: %s\n", 
                  error);
    }

    dlerror(); /* Clear existing errors */
    s->daemon_check_fds_func = dlsym(s->handle, "ais_test_daemon_check_fds");
    if ((error = dlerror()) != NULL)  {
        err_exit ("Failed to lookup ais_test_daemon_check_fds symbol: %s\n", 
                  error);
    }

    dlerror(); /* Clear existing errors */
    s->client_main = dlsym(s->handle, "saftest_driver_client_main");
    if ((error = dlerror()) != NULL)  {
        err_exit ("Failed to lookup saftest_driver_client_main symbol: %s\n", 
                  error);
    }
}

void
load_shared_libraries(char *libs_string)
{
    shared_library_t *shlib;
    char *token;

    token = strtok(libs_string, ",");
    do {
        if (NULL == token) {
            usage();
        }
        printf("token is %s\n", token);
        shlib = add_shared_library();
        strcpy(shlib->path, token);
        token = strtok(NULL, ",");
    } while (NULL != token);

    g_list_foreach(shlib_list, load_shared_library, NULL);
}

client_resource_t *
add_client_resource()
{
    client_resource_t *res;

    res = malloc(sizeof(client_resource_t));
    assert(NULL != res);
    memset(res, 0, sizeof(client_resource_t));

    client_list = g_list_append(client_list, res);
    return(res);
}

void
delete_client_resource(client_resource_t *res)
{
    client_list = g_list_remove(client_list, res);
    free(res);
}

client_resource_t *
lookup_client_resource(int fd)
{
    GList *element;
    client_resource_t *res;

    for (element = g_list_first(client_list);
         NULL != element;
         element = g_list_next(element)) {
        res = (client_resource_t *)element->data;
        if (res->client_connection_fd == fd) {
            return(res);
        }
    }
    return(NULL);
}

void
ais_test_daemon_handle_incoming_client_message(gpointer data, gpointer
user_data)
{
    void *request;
    client_resource_t *res = data;
    fd_set *fd_mask = (fd_set *)user_data;
    shared_library_t *shlib = NULL;
 

    if (NULL == data) {
        return;
    }

    if (!FD_ISSET(res->client_connection_fd, fd_mask)) {
        return;
    }

    shlib = lookup_shared_library(res->library_id);
    ais_test_log("Incoming request for library_id %s on client fd %d\n",
                 shlib->library_id, res->client_connection_fd);
    request = ais_test_recv_request(res->client_connection_fd,
                                    shlib->library_message_size);
    if (NULL == request) {
        /*
        ais_test_log("Failed to recv request from client on fd %d."
                     "Closing connection.\n",
                     res->client_connection_fd);
        */
        close(res->client_connection_fd);
        delete_client_resource(res);
        return;
    }
    shlib->daemon_handle_message_func(res->client_connection_fd,
                                      request);
}

void
ais_test_daemon_handle_shlib_add_fds(gpointer data, gpointer user_data)
{
    shared_library_t *shlib = data;
    fd_set_key_t *set_key = (fd_set_key_t *)user_data;

    if (NULL == data) {
        return;
    }

    shlib->daemon_add_fds_func(&(set_key->largest_fd), set_key->set, 
                               NULL, NULL);
}

void ais_test_daemon_add_client_to_fdset(gpointer data, gpointer user_data)
{
    client_resource_t *res;
    fd_set_key_t *set_key = (fd_set_key_t *)user_data;

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

void ais_test_daemon_add_fds(
    int *max_fd,
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    fd_set_key_t set_key;

    set_key.set = read_fd_set;
    set_key.largest_fd = *max_fd;

    g_list_foreach(client_list, ais_test_daemon_add_client_to_fdset,
                   &set_key);
    g_list_foreach(shlib_list,
                   ais_test_daemon_handle_shlib_add_fds,
                   &set_key);
    *max_fd = set_key.largest_fd;
}

void
ais_test_daemon_handle_shlib_check_fds(gpointer data, gpointer user_data)
{
    shared_library_t *shlib = data;
    fd_set *fd_mask = (fd_set *)user_data;

    if (NULL == data) {
        return;
    }

    shlib->daemon_check_fds_func(fd_mask, NULL, NULL);
}

static void
ais_test_daemon_check_fds(
    fd_set *read_fd_set,
    fd_set *write_fd_set,
    fd_set *except_fd_set)
{
    g_list_foreach(client_list,
                   ais_test_daemon_handle_incoming_client_message,
                   read_fd_set);
    g_list_foreach(shlib_list,
                   ais_test_daemon_handle_shlib_check_fds,
                   read_fd_set);
}

static void 
ais_test_daemon_fillout_read_fdset(
    int *max_fd,
    fd_set *read_fd_set)

{
    FD_ZERO(read_fd_set);
    FD_SET(daemon_listen_fd, read_fd_set);

    *max_fd = daemon_listen_fd;
    ais_test_daemon_add_fds(max_fd, read_fd_set, NULL, NULL);
}

void
ais_test_daemon_handle_accept(int daemon_listen_fd)
{
    client_resource_t *res = NULL;
    ais_test_request_t *request;

    res = add_client_resource();

    ais_test_uds_accept(daemon_listen_fd, &(res->client_connection_fd));
    request = (ais_test_request_t *)
              ais_test_recv_request(res->client_connection_fd,
                                    sizeof(ais_test_request_t));
    strcpy(res->library_id, request->library_id);
    ais_test_log("Client Connection FD %d using library id %s\n",
                 res->client_connection_fd, res->library_id);
}

static void
ais_test_daemon_select_loop(const char *socket_file)
{
    fd_set read_fd_set;
    int max_fd;
    int ret;

    ais_test_uds_listen(&daemon_listen_fd, socket_file);

    ais_test_log("Begin select loop\n");

    while (1) {
        ais_test_daemon_fillout_read_fdset(&max_fd, &read_fd_set);

        unblock_signals();
        ret = select(max_fd+1, &read_fd_set, NULL, NULL, NULL);
        block_signals();

        if (-1 == ret) {
            ais_test_log("Error %d (%s) from select()\n",
                         errno, strerror(errno));
            clear_pid_file(daemon_pid_file);
            err_exit("Exiting from select error\n");
        }
        if (0 != ret) {
            if (FD_ISSET(daemon_listen_fd, &read_fd_set)) {
                ais_test_daemon_handle_accept(daemon_listen_fd);
            } else {
                ais_test_daemon_check_fds(&read_fd_set, NULL, NULL);
            }
        } else {
            /* This is where we would put a timeout handler if we had one */
        }
    }
}

#define HELP_OPTION 1
#define DAEMON_OPTION 2
#define NO_DAEMONIZE_OPTION 3
#define SOCKET_FILE_OPTION 4
#define RUN_DIR_OPTION 5
#define LOG_FILE_OPTION 6
#define PID_FILE_OPTION 7
#define LOAD_LIBS_OPTION 8

int
main(int argc, char *argv[], char *envp[])
{
    char **argv_copy;
    int argc_copy = 0;
    int argv_ndx = 0;
    int argv_copy_ndx = 0;
    char libs_string[BUF_SIZE+1];
    int                 daemon_flag = 0;
    int                 no_daemonize_flag = 0;
    int                 socket_file_flag = 0;
    int                 run_dir_flag = 0;
    int                 log_file_flag = 0;
    int                 pid_file_flag = 0;
    int                 load_libs_flag = 0;
    char                run_path[BUF_SIZE];
    char                pid_file[BUF_SIZE];
    char                log_file[BUF_SIZE];
    char                socket_file[BUF_SIZE];
    ais_test_request_t *first_request = NULL;
    int next_option = 0;
    int is_daemon = 0;
    shared_library_t *shlib;
    GList *element;

    argc_copy = argc;
    argv_copy = (char **)malloc(sizeof(char*) * argc);
    for (argv_ndx = 0, argv_copy_ndx = 0; argv_ndx < argc; argv_ndx++) {
        if (0 == strcmp(argv[argv_ndx], "--load-libs")) {
            argc_copy -= 2;
            argv_ndx++; /* an extra one to skip the param */
            continue;
        }
        if (0 == strcmp(argv[argv_ndx], "--daemon")) {
            is_daemon = 1;
        }
        argv_copy[argv_copy_ndx] = (char *)malloc(sizeof(char) *
                                                  (strlen(argv[argv_ndx]) + 1));
        strcpy(argv_copy[argv_copy_ndx], argv[argv_ndx]);
        argv_copy_ndx++;
    }

    if (is_daemon) {
        const struct option long_options[] = {
            { "help",     0, NULL, HELP_OPTION},
            { "daemon",   0, NULL, DAEMON_OPTION},
            { "no-daemonize", 0, NULL, NO_DAEMONIZE_OPTION},
            { "load-libs", 1, NULL, LOAD_LIBS_OPTION},
            { "socket-file", 1, NULL, SOCKET_FILE_OPTION},
            { "run-dir", 1, NULL, RUN_DIR_OPTION},
            { "log-file", 1, NULL, LOG_FILE_OPTION},
            { "pid-file", 1, NULL, PID_FILE_OPTION},
            { NULL,       0, NULL, 0   }   /* Required at end of array.  */
        };

        do {
            opterr = 0;
            next_option = getopt_long (argc, argv, "", long_options, 
                                       NULL);
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
                case LOAD_LIBS_OPTION:
                    if (load_libs_flag) {
                        usage();
                    }
                    load_libs_flag++;
                    strcpy(libs_string, optarg);
                    break;
            }
        } while (-1 != next_option);
    } else {
        for (argv_ndx = 0; argv_ndx < argc; argv_ndx++) {
            if (0 == strcmp(argv[argv_ndx], "--load-libs")) {
                strcpy(libs_string, argv[argv_ndx+1]);
                load_libs_flag++;
            }
            if (0 == strcmp(argv[argv_ndx], "--run-dir")) {
                strcpy(run_path, argv[argv_ndx+1]);
                run_dir_flag++;
            }
        }
    }

    if (!load_libs_flag || ! run_dir_flag) {
        usage();
    }

    if (daemon_flag) {
        /* We are a daemon */
        if (!log_file_flag || !pid_file_flag || !socket_file_flag) {
            usage();
        }
        
        saftest_driver_init(run_path);
        load_shared_libraries(libs_string);

        daemonize(!no_daemonize_flag, pid_file);
        setup_log_file(log_file);
        ais_test_daemon_select_loop(socket_file);
    } else {
        saftest_driver_client_init(run_path);
        load_shared_libraries(libs_string);
        if (g_list_length(shlib_list) != 1) {
            err_exit("shlib count must be 1 for client mode, but it was %d\n",
                     g_list_length(shlib_list));
        }
        element = g_list_first(shlib_list);
        shlib = (shared_library_t *)element->data;

        first_request = (ais_test_request_t *)
              malloc(sizeof(ais_test_request_t));
        memset(first_request, 0, sizeof(ais_test_request_t));
        first_request->op = AIS_TEST_REQUEST_INIT;
        strcpy(first_request->library_id, shlib->library_id);

        return(shlib->client_main(argc_copy, argv_copy, 
                                  (void *)first_request,
                                  sizeof(ais_test_request_t)));
    }

    return(0);
}
