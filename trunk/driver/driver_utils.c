#include "saftest_driver.h"

static FILE *daemon_log_fp = NULL;

void
ais_test_log(const char *format, ...)
{
    va_list ap;
    va_list aq;

    char buf[BUF_SIZE+1];

    va_start(ap, format);
    /*
     * make a local copy because the pointer ap will be
     * undefined after return from vsnprintf() (Linux only)
     */
    va_copy(aq, ap);
    vsnprintf(buf, BUF_SIZE, format, aq);
    if (NULL != daemon_log_fp) {
        fprintf(daemon_log_fp, buf);
        fflush(daemon_log_fp);
    } else {
        fprintf(stdout, buf);
        fflush(stdout);
    }

    va_end(ap);
    va_end(aq);
}

void
ais_test_abort(const char *format, ...)
{
    va_list ap;
    va_list aq;

    char buf[BUF_SIZE+1];

    va_start(ap, format);
    /*
     * make a local copy because the pointer ap will be
     * undefined after return from vsnprintf() (Linux only)
     */
    va_copy(aq, ap);
    vsnprintf(buf, BUF_SIZE, format, aq);
    if (NULL != daemon_log_fp) {
        fprintf(daemon_log_fp, buf);
        fflush(daemon_log_fp);
    } else {
        fprintf(stderr, buf);
        fflush(stderr);
    }

    va_end(ap);
    va_end(aq);

    abort();
}

void
err_exit(const char *format, ...)
{
    va_list ap;
    va_list aq;

    char buf[BUF_SIZE+1];

    va_start(ap, format);
    /*
     * make a local copy because the pointer ap will be
     * undefined after return from vsnprintf() (Linux only)
     */
    va_copy(aq, ap);
    vsnprintf(buf, BUF_SIZE, format, aq);
    if (NULL != daemon_log_fp) {
        fprintf(daemon_log_fp, buf);
        fflush(daemon_log_fp);
    } else {
        fprintf(stderr, buf);
        fflush(stderr);
    }

    va_end(ap);
    va_end(aq);

    exit(255);
}

const char * get_error_string(SaAisErrorT error)
{
        switch(error) {
                case SA_AIS_OK:
                        return "SA_AIS_OK";
                case SA_AIS_ERR_LIBRARY:
                        return "SA_ERR_LIBRARY";
                case SA_AIS_ERR_VERSION:
                        return "SA_ERR_VERSION";
                case SA_AIS_ERR_INIT:
                        return "SA_ERR_INIT";
                case SA_AIS_ERR_TIMEOUT:
                        return "SA_ERR_TIMEOUT";
                case SA_AIS_ERR_TRY_AGAIN:
                        return "SA_ERR_TRY_AGAIN";
                case SA_AIS_ERR_INVALID_PARAM:
                        return "SA_ERR_INVALID_PARAM";
                case SA_AIS_ERR_NO_MEMORY:
                        return "SA_ERR_NO_MEMORY";
                case SA_AIS_ERR_BAD_HANDLE:
                        return "SA_ERR_BAD_HANDLE";
                case SA_AIS_ERR_BUSY:
                        return "SA_ERR_BUSY";
                case SA_AIS_ERR_ACCESS:
                        return "SA_ERR_ACCESS";
                case SA_AIS_ERR_NOT_EXIST:
                        return "SA_ERR_NOT_EXIST";
                case SA_AIS_ERR_NAME_TOO_LONG:
                        return "SA_ERR_NAME_TOO_LONG";
                case SA_AIS_ERR_EXIST:
                        return "SA_ERR_EXIST";
                case SA_AIS_ERR_NO_SPACE:
                        return "SA_ERR_NO_SPACE";
                case SA_AIS_ERR_INTERRUPT:
                        return "SA_ERR_INTERRUPT";
                case SA_AIS_ERR_NAME_NOT_FOUND:
                        return "SA_ERR_NAME_NOT_FOUND";
                case SA_AIS_ERR_NO_RESOURCES:
                        return "SA_ERR_NO_RESOURCES";
                case SA_AIS_ERR_NOT_SUPPORTED:
                        return "SA_ERR_NOT_SUPPORT";
                case SA_AIS_ERR_BAD_OPERATION:
                        return "SA_ERR_BAD_OPEARATION";
                case SA_AIS_ERR_FAILED_OPERATION:
                        return "SA_ERR_FAILED_OPERATION";
                case SA_AIS_ERR_MESSAGE_ERROR:
                        return "SA_ERR_MESSAGE_ERROR";
                case SA_AIS_ERR_QUEUE_FULL:
                        return "SA_ERR_QUEUE_FULL";
                case SA_AIS_ERR_QUEUE_NOT_AVAILABLE:
                        return "SA_ERR_QUEUE_NOT_AVAILABLE";
                case SA_AIS_ERR_BAD_FLAGS:
                        return "SA_ERR_BAD_FLAGS";
                case SA_AIS_ERR_TOO_BIG:
                        return "SA_AIS_ERR_TOO_BIG";
                case SA_AIS_ERR_NO_SECTIONS:
                        return "SA_AIS_ERR_NO_SECTIONS";
                default:
                        return "(invalid error code)";
        }
}

void
ais_test_uds_listen(int *fd, const char *socket_file)
{
    int optval = 1;
    int ret;
    struct sockaddr_un  sun;

    memset(&sun, 0, sizeof(sun));

    *fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == *fd) {
        err_exit("Unable to open a socket: %s\n", strerror(errno));
    }

    /*
     * Set the close-on-exec flag so that processes that are forked
     * off don't keep the file descriptor open.
     */
    ret = fcntl(*fd, F_SETFD, 1);
    if (-1 == ret) {
        close(*fd);
        err_exit("fcntl failed: %s\n", strerror(errno));
    }

    ret = setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (-1 == ret) {
        close(*fd);
        err_exit("setsockopt SO_REUSEADDR failed: %s\n", strerror(errno));
    }

    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, socket_file);
    ret = bind(*fd, (struct sockaddr *)&sun, sizeof(sun));
    if (0 != ret) {
        /*
         * The socket file exists.  Check if there is a live process
         * bound to it.  If not, remove the file and try to bind again.
         */
        if (connect(*fd, (struct sockaddr *)&sun, sizeof(sun)) == 0) {
            close(*fd);
            err_exit("Failed to bind to %s: %s\n",
                     socket_file, strerror(errno));
        }
        else {
            unlink(socket_file);
            if (bind(*fd, (struct sockaddr *)&sun, sizeof(sun)) != 0) {
                close(*fd);
                err_exit("Failed to bind to %s: %s\n",
                        socket_file, strerror(errno));
            }
        }

    }

    ret = chmod(socket_file, S_IRUSR | S_IWUSR);
    if (0 != ret) {
        close(*fd);
        unlink(socket_file);
        err_exit("Failed to chmod %s: %s\n",
                 socket_file, strerror(errno));
    }

    if (listen(*fd, SOMAXCONN) != 0) {
        close(*fd);
        unlink(socket_file);
        err_exit("Failed to listen on %s: %s\n",
                 socket_file, strerror(errno));
    }
}

void
ais_test_uds_accept(int fd, int *new_fd)
{
    int                ret;
    int                optval = 1;

    *new_fd = accept(fd, NULL, 0);
    if (-1 == *new_fd) {
        ais_test_log("Failed to accept on fd %d: %s\n",
                     fd, strerror(errno));
    }

    /*
     * Turn on keep-alive so that we can detect when the remote
     * goes away while we are waiting for a reply message
     */
    do {
        ret = setsockopt(*new_fd, SOL_SOCKET, SO_KEEPALIVE,
                         &optval, sizeof(optval));
        if (-1 == ret && ret != EINTR) {
            close(*new_fd);
            err_exit("setsockopt KEEPALIVE failed: %s\n", strerror(errno));
        }
    } while (-1 == ret);
}

void
ais_test_uds_connect(int *fd, const char *socket_path)
{
    struct sockaddr_un  sun;
    int                ret;
    int                optval = 1;

    memset(&sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, socket_path);

    *fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (-1 == *fd) {
        err_exit("Unable to open a socket: %s\n", strerror(errno));
    }

    ret = connect(*fd, (struct sockaddr *)&sun, sizeof(sun));
    if (-1 == ret) {
        close(*fd);
        err_exit("Unable to connect to %s: %s\n",
                 socket_path, strerror(errno));
    }

    do {
        ret = setsockopt(*fd, SOL_SOCKET, SO_KEEPALIVE,
                         &optval, sizeof(optval));
        if (-1 == ret && ret != EINTR) {
            close(*fd);
            err_exit("setsockopt KEEPALIVE failed: %s\n", strerror(errno));
        }
    } while (-1 == ret);
}

void *
ais_test_recv_request(int client_connection_fd, int expected_length)
{
    void *request;
    int bytes_recvd;
    int bytes_left;
    int counter = 0;

    request = malloc(expected_length);
    assert(NULL != request);

    bytes_left = expected_length;
    for (counter = 0; (bytes_left > 0) && (counter < RECV_RETRIES); counter++) {
        bytes_recvd = recv(client_connection_fd, request, bytes_left,
                        MSG_WAITALL | MSG_NOSIGNAL);
        bytes_left -= bytes_recvd;
    }
    if (bytes_left > 0) {
        /*
        ais_test_log("Error receiving on socket %d: "
                     "Expected %d bytes, received %d bytes\n",
                     res->client_connection_fd,
                     expected_length,
                     expected_length - bytes_left);
        */
        free(request);
        request = NULL;
    }
    return(request);
}

void *
ais_test_recv_reply(int fd, int expected_length)
{
    void *reply;
    int bytes_recvd;

    reply = malloc(expected_length);
    assert(NULL != reply);

    bytes_recvd = recv(fd, reply, expected_length,
                       MSG_WAITALL | MSG_NOSIGNAL);
    if (bytes_recvd < expected_length) {
        ais_test_log("Error receiving on socket %d: "
                     "Expected %d bytes, received %d bytes\n",
                     fd, expected_length, bytes_recvd);
        free(reply);
        reply = NULL;
    }
    return(reply);
}

void *
ais_test_send_request(int fd, void *request, 
                      int request_length, int expected_reply_length)
{
    int bytes_sent;
    void *reply = NULL;

    assert(NULL != request);

    bytes_sent = send(fd, request, request_length, MSG_NOSIGNAL);
    if (bytes_sent < request_length) {
        ais_test_abort("Error sending on socket %d: "
                       "Expected %d bytes, sent %d bytes\n",
                       fd, request_length, bytes_sent);
    }
    if (expected_reply_length > 0) {
        reply = ais_test_recv_reply(fd, expected_reply_length);
    }
    return(reply);
}

void
ais_test_send_reply(int client_connection_fd, void *reply, int reply_length)
{
    int bytes_sent;

    assert(NULL != reply);

    bytes_sent = send(client_connection_fd, reply,
                      reply_length, MSG_NOSIGNAL);
    if (bytes_sent < reply_length) {
        ais_test_abort("Error sending on socket %d: "
                       "Expected %d bytes, sent %d bytes\n",
                       client_connection_fd,
                       reply_length, bytes_sent);
    }
}

void
block_signals()
{
    sigset_t             sigset;

    sigfillset(&sigset);
    sigdelset(&sigset, SIGABRT);
    sigdelset(&sigset, SIGSEGV);
    sigdelset(&sigset, SIGINT);
    if (sigprocmask(SIG_SETMASK, &sigset, NULL) != 0) {
        ais_test_abort("Error blocking signals\n");
    }
}

void
unblock_signals()
{
    sigset_t             sigset;

    sigemptyset(&sigset);
    if (sigprocmask(SIG_SETMASK, &sigset, NULL) != 0) {
        ais_test_abort("Error unblocking signals\n");
    }
}

void
setup_core_dump(const char *run_dir)
{
    struct rlimit lim;
    int ret = 0;

    lim.rlim_cur = RLIM_INFINITY;
    lim.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &lim);

    ret = chdir(run_dir);
    if (ret != 0) {
        err_exit("Unable to change to run directory %s: %s\n",
                 run_dir, strerror(errno));
    }
}

int
write_pid_file(const char *pid_file)
{
    FILE *fp;
    pid_t pid;
    int chmod_status;

    fp = fopen(pid_file, "w");
    if (fp == NULL) {
        ais_test_log("Failed to open %s: %s\n",
                           pid_file, strerror(errno));
        return(-1);
    }
    chmod_status = chmod(pid_file, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (chmod_status != 0) {
        ais_test_log("Failed to change permissions on %s: %s\n",
                           pid_file, strerror(errno));
        return(-1);
    }
    pid = getpid();
    fprintf(fp, "%u\n", pid);
    fclose(fp);
    return(0);
}

void
clear_pid_file(const char *pid_file)
{
    unlink(pid_file);
}

void
daemonize(int want_fork, const char *pid_file)
{
    pid_t pid;
    int fd;

    if (want_fork) {
        if ((pid = fork()) < 0) {
            err_exit("cannot fork into background: %s\n",
                    strerror(errno));
        }

        /*
        * parent
        */
        if (pid) {
            exit(0);
        }
    }

    (void) setsid();

    write_pid_file(pid_file);

    /*
     * We're the child.
     *
     * Close all open file descriptors
     */
    for (fd = 0; fd < getdtablesize(); fd++) {
        close(fd);
    }

    /*
     * Open the standard io descriptors (stdin, stdout, stderr)
     * to a harmless device.  So, if some misguided routine
     * attempts to write to one of them while we are in the background,
     * nothing will happen.
     */

    /*
     * This will be fd 0, stdin.
     */
    fd = open("/dev/null", O_RDWR);

    if (fd < 0) {
        err_exit("cannot open /dev/null i/o: %s\n",
                 strerror(errno));
    }

    if (dup2(fd, 1) == -1) {
        err_exit("cannot redirect standard i/o: %s\n",
                 strerror(errno));
    }

    if (dup2(fd, 2) == -1) {
        err_exit("cannot redirect standard i/o: %s\n",
                 strerror(errno));
    }

    block_signals();
}

void setup_log_file(const char *log_file)
{
    daemon_log_fp = fopen(log_file, "a");
    if (NULL == daemon_log_fp) {
        err_exit("Unable to open log file %s\n",
                log_file);
    }
}

void saftest_driver_init(const char *run_path)
{
    setup_core_dump(run_path);
}

void saftest_driver_client_init(const char *run_path)
{
    saftest_driver_init(run_path);
}

