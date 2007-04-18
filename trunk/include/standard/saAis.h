/*******************************************************************************
**
** FILE:
**   SaAis.h
**
** DESCRIPTION: 
**   This file contains the prototypes and type definitions required by all
**   Service Availability(TM) Forum's AIS services. 
**   
** SPECIFICATION VERSION:
**   SAI-Overview-B.02.01
**
** DATE: 
**   Fri  Nov   18  2005  
**
** LEGAL:
**   OWNERSHIP OF SPECIFICATION AND COPYRIGHTS. 
**   The Specification and all worldwide copyrights therein are
**   the exclusive property of Licensor.  You may not remove, obscure, or
**   alter any copyright or other proprietary rights notices that are in or
**   on the copy of the Specification you download.  You must reproduce all
**   such notices on all copies of the Specification you make.  Licensor
**   may make changes to the Specification, or to items referenced therein,
**   at any time without notice.  Licensor is not obligated to support or
**   update the Specification. 
**   
**   Copyright(c) 2005, Service Availability(TM) Forum. All rights
**   reserved. 
**
*******************************************************************************/

#ifndef _SA_AIS_H
#define _SA_AIS_H

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
    SA_FALSE = 0,
    SA_TRUE = 1
} SaBoolT;

typedef char                  SaInt8T;
typedef short                 SaInt16T;
typedef long                  SaInt32T;
typedef long long             SaInt64T;
typedef unsigned char         SaUint8T;
typedef unsigned short        SaUint16T;
typedef unsigned long         SaUint32T;
typedef unsigned long long    SaUint64T;

/** Types used by the NTF/IMMS service **/
typedef float                 SaFloatT;
typedef double                SaDoubleT;
typedef char*                 SaStringT;


typedef SaInt64T              SaTimeT;
typedef SaUint64T             SaInvocationT;
typedef SaUint64T             SaSizeT;
typedef SaUint64T             SaOffsetT;
typedef SaUint64T             SaSelectionObjectT;

typedef struct {				
   SaSizeT   bufferSize;
   SaUint8T  *bufferAddr;
} SaAnyT;

/*
 * I added the "LL" to these to work around compiler warnings.  SAF already
 * has the errata and sayan is working on fixing this in the published header
 * files.
 */
#define SA_TIME_END              0x7FFFFFFFFFFFFFFFLL
#define SA_TIME_BEGIN            0x0LL
#define SA_TIME_UNKNOWN          0x8000000000000000LL

#define SA_TIME_ONE_MICROSECOND 1000LL
#define SA_TIME_ONE_MILLISECOND 1000000LL
#define SA_TIME_ONE_SECOND      1000000000LL
#define SA_TIME_ONE_MINUTE      60000000000LL
#define SA_TIME_ONE_HOUR        3600000000000LL
#define SA_TIME_ONE_DAY         86400000000000LL
#define SA_TIME_MAX             SA_TIME_END

#define SA_MAX_NAME_LENGTH 256

typedef struct {
    SaUint16T length;
    SaUint8T value[SA_MAX_NAME_LENGTH];
} SaNameT;

typedef struct {
    SaUint8T releaseCode;
    SaUint8T majorVersion;
    SaUint8T minorVersion;
} SaVersionT;

#define SA_TRACK_CURRENT 0x01
#define SA_TRACK_CHANGES 0x02
#define SA_TRACK_CHANGES_ONLY 0x04

typedef enum {
    SA_DISPATCH_ONE = 1,
    SA_DISPATCH_ALL = 2,
    SA_DISPATCH_BLOCKING = 3
} SaDispatchFlagsT;

typedef enum {
   SA_AIS_OK = 1,
   SA_AIS_ERR_LIBRARY = 2,
   SA_AIS_ERR_VERSION = 3,
   SA_AIS_ERR_INIT = 4,
   SA_AIS_ERR_TIMEOUT = 5,
   SA_AIS_ERR_TRY_AGAIN = 6,
   SA_AIS_ERR_INVALID_PARAM = 7,
   SA_AIS_ERR_NO_MEMORY = 8,
   SA_AIS_ERR_BAD_HANDLE = 9,
   SA_AIS_ERR_BUSY = 10,
   SA_AIS_ERR_ACCESS = 11,
   SA_AIS_ERR_NOT_EXIST = 12,
   SA_AIS_ERR_NAME_TOO_LONG = 13,
   SA_AIS_ERR_EXIST = 14,
   SA_AIS_ERR_NO_SPACE = 15,
   SA_AIS_ERR_INTERRUPT =16,
   SA_AIS_ERR_NAME_NOT_FOUND = 17,
   SA_AIS_ERR_NO_RESOURCES = 18,
   SA_AIS_ERR_NOT_SUPPORTED = 19,
   SA_AIS_ERR_BAD_OPERATION = 20,
   SA_AIS_ERR_FAILED_OPERATION = 21,
   SA_AIS_ERR_MESSAGE_ERROR = 22,
   SA_AIS_ERR_QUEUE_FULL = 23,
   SA_AIS_ERR_QUEUE_NOT_AVAILABLE = 24,
   SA_AIS_ERR_BAD_FLAGS = 25,
   SA_AIS_ERR_TOO_BIG = 26,
   SA_AIS_ERR_NO_SECTIONS = 27,
   SA_AIS_ERR_NO_OP = 28,          
   SA_AIS_ERR_REPAIR_PENDING = 29
} SaAisErrorT;

typedef enum {
 SA_SVC_HPI  =  1,
 SA_SVC_AMF  =  2,
 SA_SVC_CLM  =  3,
 SA_SVC_CKPT =  4,
 SA_SVC_EVT  =  5,
 SA_SVC_LCK  =  6,
 SA_SVC_MSG  =  7,
 SA_SCV_LOG  =  8,
 SA_SVC_NTF  =  9,
 SA_SVC_IMMS = 10
} SaServicesT;


#ifdef  __cplusplus
}
#endif

#endif  /* _SA_AIS_H */

