#ifndef SAFTEST_LOG_H
#define SAFTEST_LOG_H

/**********************************************************************
 *
 *  I N C L U D E S
 *
 **********************************************************************/

#include "saftest_common.h"

extern void
ais_test_log_set_fp(FILE *fp);

extern void
ais_test_log(const char *format, ...);

extern void
ais_test_abort(const char *format, ...);

extern void
err_exit(const char *format, ...);


#endif /* SAFTEST_LOG_H */
