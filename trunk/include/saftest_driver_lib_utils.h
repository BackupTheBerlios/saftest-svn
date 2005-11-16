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
saftest_dispatch_flags_from_string(const char *dispatch_flags_str);

extern const char *
saftest_dispatch_flags_to_string(SaDispatchFlagsT dispatch_flags);

#endif /* SAFTEST_DRIVER_LIB_UTILS_H */
