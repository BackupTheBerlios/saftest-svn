#ifndef SAFTEST_COMMON_H
#define SAFTEST_COMMON_H

/**********************************************************************
 *
 *  I N C L U D E S
 *
 **********************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <assert.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <elf.h>
#include <dlfcn.h>
#include "glib.h"

/**********************************************************************
 *
 *  D E F I N E S
 *
 **********************************************************************/
#define BUF_SIZE 2048
#define RECV_RETRIES 5

#define AIS_B_VERSION_MAJOR 0x01
#define AIS_B_VERSION_MINOR 0x01
#define AIS_B_RELEASE_CODE 'B'

typedef struct fd_set_key {
    fd_set *set;
    int largest_fd;
} fd_set_key_t;

/**********************************************************************
 *
 *  G L O B A L S
 *
 **********************************************************************/

extern char    *optarg;
extern int      optind;

typedef unsigned char ubit8;
typedef unsigned short ubit16;
typedef unsigned int ubit32;
typedef signed long long sbit64;
typedef unsigned long long ubit64;

#define UBIT8_MAX UCHAR_MAX
#define UBIT16_MAX USHRT_MAX
#define UBIT32_MAX UINT_MAX
#define SBIT64_MAX LLONG_MAX
#define UBIT64_MAX ULLONG_MAX

#endif /* SAFTEST_COMMON_H */
