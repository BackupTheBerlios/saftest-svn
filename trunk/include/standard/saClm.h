/*******************************************************************************
**
** FILE:
**   SaClm.h
**
** DESCRIPTION: 
**   This file provides the C language binding for the Service 
**   Availability(TM) Forum AIS Cluster Membership Service (CLM). It contains  
**   all the prototypes and type definitions required for CLM. 
**   
** SPECIFICATION VERSION:
**   SAI-AIS-CLM-B.02.01
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


#ifndef _SA_CLM_H
#define _SA_CLM_H

#include <saAis.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef SaUint64T SaClmHandleT;
typedef SaUint32T SaClmNodeIdT;

#define SA_CLM_LOCAL_NODE_ID 0XFFFFFFFF

#define SA_CLM_MAX_ADDRESS_LENGTH 64

typedef enum {
         SA_CLM_AF_INET = 1,
         SA_CLM_AF_INET6 = 2
} SaClmNodeAddressFamilyT;

typedef struct {
         SaClmNodeAddressFamilyT family;
         SaUint16T length;
         SaUint8T value[SA_CLM_MAX_ADDRESS_LENGTH];
} SaClmNodeAddressT;

typedef struct {
         SaClmNodeIdT nodeId;
         SaClmNodeAddressT nodeAddress;
         SaNameT nodeName;
         SaBoolT member;
         SaTimeT bootTimestamp;
         SaUint64T initialViewNumber;
} SaClmClusterNodeT;

typedef enum {
         SA_CLM_NODE_NO_CHANGE = 1,
         SA_CLM_NODE_JOINED = 2,
         SA_CLM_NODE_LEFT = 3,
         SA_CLM_NODE_RECONFIGURED = 4
} SaClmClusterChangesT;

typedef struct {
         SaClmClusterNodeT clusterNode;
         SaClmClusterChangesT clusterChange;
} SaClmClusterNotificationT;

typedef struct {
         SaUint64T viewNumber;
         SaUint32T numberOfItems;
         SaClmClusterNotificationT *notification;
} SaClmClusterNotificationBufferT;

typedef void (*SaClmClusterTrackCallbackT) (
         const SaClmClusterNotificationBufferT *notificationBuffer,
         SaUint32T numberOfMembers,
         SaAisErrorT error);

typedef void (*SaClmClusterNodeGetCallbackT) (
         SaInvocationT invocation,
         const SaClmClusterNodeT *clusterNode,
         SaAisErrorT error);

typedef struct {
         SaClmClusterNodeGetCallbackT saClmClusterNodeGetCallback;
         SaClmClusterTrackCallbackT saClmClusterTrackCallback;
} SaClmCallbacksT;

typedef enum {
   SA_CLM_CLUSTER_CHANGE_STATUS = 1
} SaClmStateT;

/*************************************************/
/******** CLM API function declarations **********/
/*************************************************/
    extern SaAisErrorT 
saClmInitialize(SaClmHandleT *clmHandle, const SaClmCallbacksT *clmCallbacks,
                SaVersionT *version);
    extern SaAisErrorT 
saClmSelectionObjectGet(SaClmHandleT clmHandle, 
                        SaSelectionObjectT *selectionObject);
    extern SaAisErrorT
saClmDispatch(SaClmHandleT clmHandle, 
              SaDispatchFlagsT dispatchFlags);
    extern SaAisErrorT 
saClmFinalize(SaClmHandleT clmHandle);
    extern SaAisErrorT 
saClmClusterTrack(SaClmHandleT clmHandle,
                  SaUint8T trackFlags,
                  SaClmClusterNotificationBufferT *notificationBuffer
);
    extern SaAisErrorT 
saClmClusterTrackStop(SaClmHandleT clmHandle);
	extern SaAisErrorT 
saClmClusterNotificationFree(SaClmHandleT clmHandle,
                             SaClmClusterNotificationT *notification
);
    extern SaAisErrorT 
saClmClusterNodeGet(SaClmHandleT clmHandle,
                    SaClmNodeIdT nodeId, 
                    SaTimeT timeout,
                    SaClmClusterNodeT *clusterNode);
    extern SaAisErrorT
saClmClusterNodeGetAsync(SaClmHandleT clmHandle,
                         SaInvocationT invocation,
                         SaClmNodeIdT nodeId);

#ifdef  __cplusplus
}
#endif

#endif  /* _SA_CLM_H */



