#include "saftest_common.h"
#include "saftest_driver.h"
#include "saftest_log.h"
#include "saftest_comm.h"

saftest_list shlib_list = NULL;
static char  daemon_pid_file[BUF_SIZE];
static FILE *daemon_log_fp = NULL;

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
    printf("                        [--no-daemonize]\n");

    printf("\n");
    saftest_abort("blah\n");
    exit(255);
}

shared_library_t *
add_shared_library()
{
    shared_library_t *shlib;

    shlib = malloc(sizeof(shared_library_t));
    assert(NULL != shlib);
    memset(shlib, 0, sizeof(shared_library_t));

    saftest_list_element_create(shlib_list, shlib);
    return(shlib);
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

void
load_shared_library(void *data, void *key)
{
    shared_library_t *s;
    int           fd;
    Elf32_Ehdr    elfhdr;
    struct stat   stat_buf;
    char          *error_str;
    int           load_opt;
    const char * (*get_library_id_fn)(void);
    char *error;
    char buf[SAFTEST_STRING_LENGTH];

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

    /*load_opt = RTLD_LAZY;*/
    load_opt = RTLD_NOW;

    s->handle = dlopen(s->path, load_opt);
    error_str = dlerror();

    if (s->handle == NULL) {
        err_exit("Failed to load shared library %s: %s\n", s->path, error_str);
    }

    s->loaded = 1;

    dlerror(); /* Clear existing errors */
    
    get_library_id_fn = 
        (const char* (*)(void))dlsym(s->handle, "get_library_id");
    if ((error = dlerror()) != NULL)  {
        err_exit("Failed to lookup get_library_id symbol: %s\n", error);
    }
    strcpy(s->library_id, get_library_id_fn());
    dlerror();
    
    s->daemon_init_func = (void (*) ()) dlsym(s->handle, "saftest_daemon_init");
    if ((error = dlerror()) != NULL)  {
        err_exit("Failed to lookup saftest_daemon_init symbol: %s\n", error);
    }
    if (NULL != daemon_log_fp) {
        s->daemon_init_func(daemon_log_fp);
    }

    dlerror();
    s->daemon_thread_init_func = 
        (void (*) ())dlsym(s->handle, "saftest_daemon_thread_init");
    if ((error = dlerror()) != NULL)  {
        err_exit("Failed to lookup "
                 "saftest_daemon_thread_init symbol: %s\n", error);
    }

    dlerror();
    s->daemon_add_fds_func = 
        (void (*) ())dlsym(s->handle, "saftest_daemon_add_fds");
    if ((error = dlerror()) != NULL)  {
        err_exit("Failed to lookup saftest_daemon_add_fds symbol: %s\n", error);
    }

    dlerror();
    s->daemon_check_fds_func = 
        (void (*)())dlsym(s->handle, "saftest_daemon_check_fds");
    if ((error = dlerror()) != NULL)  {
        err_exit("Failed to lookup saftest_daemon_check_fds symbol: %s\n", 
                  error);
    }
    dlerror();

    /* We only load this function for the main_lib "MAIN" */
    if (0 == strcmp(s->library_id, "MAIN")) {
        s->main_lib_main_func = 
            (void (*) ())dlsym(s->handle, "saftest_daemon_main_lib_main");
        if ((error = dlerror()) != NULL)  {
            err_exit("Failed to lookup saftest_daemon_main_lib_main "
                     "symbol: %s\n", error);
        }
        dlerror();
    }

    /* 
     * The SAFTEST_MAP_TABLE_END macro defines a function called
     * saftest_driver_get_map_table_LIBID()
     * 
     * for example: saftest_driver_get_map_table_CLM()
     */ 
    sprintf(buf, "saftest_driver_get_map_table_%s", s->library_id);
    s->get_map_table_func = (saftest_map_table_entry_t * (*)())dlsym(s->handle, buf);

    if ((error = dlerror()) != NULL)  {
        err_exit ("Failed to lookup saftest_driver_get_map_table symbol: %s\n", 
                  error);
    }
    /* saftest_assert(s->get_map_table_func != saftest_driver_get_map_table_MAIN,
                   "Invalid get_map_table_func pointer\n"); */
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
        /*printf("token is %s\n", token);*/
        shlib = add_shared_library();
        strcpy(shlib->path, token);
        token = strtok(NULL, ",");
    } while (NULL != token);

    saftest_list_each(shlib_list, load_shared_library, NULL);
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

#define HELP_OPTION 1
#define DAEMON_OPTION 2
#define LOAD_LIBS_OPTION 3
#define NO_DAEMONIZE_OPTION 4
#define SOCKET_FILE_OPTION 5
#define RUN_DIR_OPTION 6
#define LOG_FILE_OPTION 7
#define PID_FILE_OPTION 8
#define OP_NAME_OPTION 9
#define KEY_OPTION 10
#define VALUE_OPTION 11

static void
saftest_driver_free_argv_copy(int argc, char **argv_copy)
{
    int argv_ndx = 0;

    for (argv_ndx = 0; argv_ndx < argc; argv_ndx++) {
        free(argv_copy[argv_ndx]);
    }
    free(argv_copy);
}

int
saftest_driver_client_main(shared_library_t *shlib, int argc, char **argv)
{
    SaAisErrorT         status = 255;
    int                 daemon_flag = 0;
    int                 no_daemonize_flag = 0;
    int                 socket_file_flag = 0;
    int                 run_dir_flag = 0;
    int                 log_file_flag = 0;
    int                 pid_file_flag = 0;
    int                 op_name_flag = 0;
    char                run_path[BUF_SIZE];
    char                pid_file[BUF_SIZE];
    char                log_file[BUF_SIZE];
    char                socket_file[BUF_SIZE];
    char                op_name[BUF_SIZE];
    char                key[SAFTEST_STRING_LENGTH+1];
    char                value[SAFTEST_STRING_LENGTH+1];
    saftest_map_table_entry_t *entry;
    int                 next_option = 0;
    int                 client_fd;
    saftest_msg_t      *request = NULL;

    const struct option long_options[] = {
        { "help",     0, NULL, HELP_OPTION},
        { "daemon",   0, NULL, DAEMON_OPTION},
        { "no-daemonize", 0, NULL, NO_DAEMONIZE_OPTION},
        { "socket-file", 1, NULL, SOCKET_FILE_OPTION},
        { "run-dir", 1, NULL, RUN_DIR_OPTION},
        { "log-file", 1, NULL, LOG_FILE_OPTION},
        { "pid-file", 1, NULL, PID_FILE_OPTION},
        { "op", 1, NULL, OP_NAME_OPTION},
        { "key", 1, NULL, KEY_OPTION},
        { "value", 1, NULL, VALUE_OPTION},
        { NULL,       0, NULL, 0   }   /* Required at end of array.  */
    };

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
                request = saftest_request_msg_create(op_name);
                break;
            case KEY_OPTION:
                strcpy(key, optarg);
                break;
            case VALUE_OPTION:
                strcpy(value, optarg);
                assert(NULL != request);
                saftest_msg_set_str_value(request, key, value);
                break;
            case -1:
                /* No more options */
                break;
            default:
                usage();
        } /* switch (c) */
    } while (-1 != next_option);

    saftest_driver_free_argv_copy(argc, argv);

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

    assert(NULL != shlib);

    saftest_uds_connect(&client_fd, socket_file);

    entry = saftest_get_map_table_entry(shlib->get_map_table_func(), op_name);
    if (NULL == entry) {
       saftest_abort("Unable to find %s map table entry for op %s.\n",
                     shlib->library_id, op_name);
    }

    status = entry->client_handler(client_fd, request);
    saftest_msg_free(&request);

    return(status);
}

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
    char                log_file[BUF_SIZE];
    char                socket_file[BUF_SIZE];
    int next_option = 0;
    int is_daemon = 0;
    shared_library_t *shlib;
    saftest_list_element element;

    shlib_list = saftest_list_create();

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
                    strcpy(daemon_pid_file, optarg);
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

    if (!run_dir_flag) {
        usage();
    }

    if (daemon_flag) {
        saftest_driver_free_argv_copy(argc_copy, argv_copy);
        /* We are a daemon */
        if (!load_libs_flag || !log_file_flag || 
            !pid_file_flag || !socket_file_flag) {
            usage();
        }
        
        saftest_driver_init(run_path);
        daemonize(!no_daemonize_flag, daemon_pid_file);
        daemon_log_fp = setup_log_file(log_file);
        load_shared_libraries(libs_string);

        shlib = lookup_shared_library("MAIN");
        saftest_assert(NULL != shlib,
                       "We must have a MAIN lib if we're a daemon\n");
        shlib->main_lib_main_func(shlib_list, socket_file, daemon_pid_file);
    } else {
        saftest_driver_client_init(run_path);
        if (load_libs_flag) {
            load_shared_libraries(libs_string);
            if (saftest_list_size(shlib_list) != 1) {
                err_exit("shlib count must be 1 for client mode, "
                         "but it was %d\n",
                         saftest_list_size(shlib_list));
            }
            element = saftest_list_first(shlib_list);
            shlib = (shared_library_t *)element->data;
        } else {
            shlib = NULL;
        }

        return(saftest_driver_client_main(shlib, argc_copy, argv_copy));
    }

    return(0);
}

