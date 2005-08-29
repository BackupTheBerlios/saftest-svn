#ifndef SAFTEST_DRIVER_LIB_UTILS_H
#define SAFTEST_DRIVER_LIB_UTILS_H

/**********************************************************************
 *
 *  I N C L U D E S
 *
 **********************************************************************/

#include "saftest_common.h"
#include "saftest_comm.h"
#include "saAis.h"

/**********************************************************************
 *
 *  D E F I N E S
 *
 **********************************************************************/

/**********************************************************************
 *
 *  G L O B A L S
 *
 **********************************************************************/

extern const char *
get_error_string(SaAisErrorT error);

extern SaDispatchFlagsT
saftest_daemon_get_dispatch_flags(const char *dispatch_flags_str);


#endif /* SAFTEST_DRIVER_LIB_UTILS_H */
