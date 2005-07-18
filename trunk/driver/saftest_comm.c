#include "saftest_common.h"
#include "saftest_comm.h"
#include "saftest_log.h"

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
    for (counter = 0; (bytes_left > 0) && (counter < RECV_RETRIES); counter++)
{
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

