#include "saftest_common.h"
#include "saftest_log.h"
#include "saftest_driver_lib_utils.h"

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

SaDispatchFlagsT
saftest_daemon_get_dispatch_flags(const char *dispatch_flags_str)
{
    SaDispatchFlagsT flags;

    if (0 == strcmp(dispatch_flags_str, "SA_DISPATCH_ONE")) {
        flags = SA_DISPATCH_ONE;
    } else if (0 == strcmp(dispatch_flags_str, "SA_DISPATCH_ALL")) {
        flags = SA_DISPATCH_ALL;
    } else if (0 == strcmp(dispatch_flags_str, "SA_DISPATCH_BLOCKING")) {
        flags = SA_DISPATCH_BLOCKING;
    } else if (0 == strcmp(dispatch_flags_str, "SA_DISPATCH_INVALID")) {
        flags = -1;
    } else {
        saftest_abort("Unknown dispatch flags string %s\n",
                      dispatch_flags_str);
    }
    return(flags);
}
