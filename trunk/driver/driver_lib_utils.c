#include "saftest_common.h"
#include "saftest_system.h"
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
saftest_dispatch_flags_from_string(const char *dispatch_flags_str)
{
    SaDispatchFlagsT flags;

    /* 
     * This is a sanity check.  These should start at 1, but we just want to
     * be sure since we will use 0 for the case where there is no automatic
     * dispatching.
     */
    saftest_assert((SAFTEST_DISPATCH_NONE != SA_DISPATCH_ONE) &&
                   (SAFTEST_DISPATCH_NONE != SA_DISPATCH_ALL) && 
                   (SAFTEST_DISPATCH_NONE != SA_DISPATCH_BLOCKING),
                   "Why did this happen?");

    if (0 == strcmp(dispatch_flags_str, "SA_DISPATCH_ONE")) {
        flags = SA_DISPATCH_ONE;
    } else if (0 == strcmp(dispatch_flags_str, "SA_DISPATCH_ALL")) {
        flags = SA_DISPATCH_ALL;
    } else if (0 == strcmp(dispatch_flags_str, "SA_DISPATCH_BLOCKING")) {
        flags = SA_DISPATCH_BLOCKING;
    } else if (0 == strcmp(dispatch_flags_str, "SAFTEST_DISPATCH_NONE")) {
        flags = SAFTEST_DISPATCH_NONE;
    } else if (0 == strcmp(dispatch_flags_str, "SA_DISPATCH_INVALID")) {
        flags = -1;
    } else {
        saftest_abort("Unknown dispatch flags string %s\n",
                      dispatch_flags_str);
    }
    return(flags);
}

const char *
saftest_dispatch_flags_to_string(SaDispatchFlagsT dispatch_flags)
{
    if (SA_DISPATCH_ONE == dispatch_flags) {
        return("SA_DISPATCH_ONE");
    } else if (SA_DISPATCH_ALL == dispatch_flags) {
        return("SA_DISPATCH_ALL");
    } else if (SA_DISPATCH_BLOCKING == dispatch_flags) {
        return("SA_DISPATCH_BLOCKING");
    } else if (SAFTEST_DISPATCH_NONE == dispatch_flags) {
        return("SAFTEST_DISPATCH_NONE");
    } else {
        saftest_abort("Unknown dispatch flags %d\n",
                      dispatch_flags);
    }
    return(NULL);
}

SaTimeT
saftest_time_from_string(const char *time_str)
{
    SaTimeT time;

    if (0 == strcmp(time_str, "SA_TIME_BEGIN")) {
        time = SA_TIME_BEGIN;
    } else if (0 == strcmp(time_str, "SA_TIME_END")) {
        time = SA_TIME_END;
    } else if (0 == strcmp(time_str, "SA_TIME_UNKNOWN")) {
        time = SA_TIME_UNKNOWN;
    } else if (0 == strcmp(time_str, "SA_TIME_ONE_MICROSECOND")) {
        time = SA_TIME_ONE_MICROSECOND;
    } else if (0 == strcmp(time_str, "SA_TIME_ONE_MILLISECOND")) {
        time = SA_TIME_ONE_MILLISECOND;
    } else if (0 == strcmp(time_str, "SA_TIME_ONE_SECOND")) {
        time = SA_TIME_ONE_SECOND;
    } else if (0 == strcmp(time_str, "SA_TIME_ONE_MINUTE")) {
        time = SA_TIME_ONE_MINUTE;
    } else if (0 == strcmp(time_str, "SA_TIME_ONE_HOUR")) {
        time = SA_TIME_ONE_HOUR;
    } else if (0 == strcmp(time_str, "SA_TIME_ONE_DAY")) {
        time = SA_TIME_ONE_DAY;
    } else if (0 == strcmp(time_str, "SA_TIME_MAX")) {
        time = SA_TIME_MAX;
    } else {
        time = SAFTEST_STRTOULL(time_str, NULL, 0);
    }

    return(time);
}
