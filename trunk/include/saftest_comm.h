#ifndef SAFTEST_COMM_H
#define SAFTEST_COMM_H

/**********************************************************************
 *
 *  I N C L U D E S
 *
 **********************************************************************/

#include "saftest_common.h"

extern void
ais_test_uds_listen(int *fd, const char *socket_file);

extern void
ais_test_uds_accept(int fd, int *new_fd);

extern void
ais_test_uds_connect(int *fd, const char *socket_path);

extern void
ais_test_uds_connect(int *fd, const char *socket_path);

extern void *
ais_test_send_request(int fd, void *request,
                      int request_length, int expected_reply_length);

extern void
ais_test_send_reply(int client_connection_fd, void *reply, int reply_length);

extern void *
ais_test_recv_request(int client_connection_fd, int expected_length);

extern void *
ais_test_recv_reply(int fd, int expected_length);

#endif /* SAFTEST_COMM_H */
