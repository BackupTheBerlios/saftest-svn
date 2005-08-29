module LCKTestDriver

require 'SAFTestDriver'
require 'SAFTestUtils'

class LCKTestDriver < SAFTestDriver::SAFTestDriver
    @@nextInstanceID = 1

    def initialize(node)
        driverLib = "%s/AIS-lck-%s/driver/lck_driver.so" % \
                    [ENV['SAFTEST_ROOT'],
                     SAFTestUtils::SAFTestUtils.getAISLibVersion()]
        instanceID = @@nextInstanceID
        @@nextInstanceID += 1
        super(node, driverLib, instanceID)
    end

    def getName()
        return 'lck_driver'
    end

    def init(resourceID, dispatchFlags, expectedReturn)
        kvp_hash = {'LCK_RESOURCE_ID' => resourceID,
                    'DISPATCH_FLAGS' => dispatchFlags,
                    'LOCK_GRANT_CB' => 'FALSE',
                    'RESOURCE_UNLOCK_CB' => 'FALSE',
                    'LOCK_WAITER_CB' => 'FALSE',
                    'VERSION_RELEASE_CODE' => 
                        SAFTestUtils::SAFTestUtils.SA_AIS_RELEASE_CODE,
                    'VERSION_MAJOR' => 
                        SAFTestUtils::SAFTestUtils.SA_AIS_MAJOR_VERSION,
                    'VERSION_MINOR' => 
                        SAFTestUtils::SAFTestUtils.SA_AIS_MINOR_VERSION,
                    'DISPATCH_FLAGS' => dispatchFlags,
                    'NULL_LCK_HANDLE' => 'FALSE',
                    'NULL_CALLBACKS' => 'FALSE',
                    'NULL_VERSION' => 'FALSE'}
        runDriver("INITIALIZE_REQ", kvp_hash, expectedReturn)
    end

    def initWithOptions(resourceID, dispatchFlags, 
                        releaseCode, majorVersion, minorVersion,
                        nullLckHandle, nullCallbacks, nullVersion,
                        expectedReturn)
        kvp_hash = {'LCK_RESOURCE_ID' => resourceID,
                    'DISPATCH_FLAGS' => dispatchFlags,
                    'VERSION_RELEASE_CODE' => releaseCode,
                    'VERSION_MAJOR' => majorVersion,
                    'VERSION_MINOR' => minorVersion,
                    'DISPATCH_FLAGS' => dispatchFlags,
                    'NULL_CALLBACKS' => 'FALSE',
                    'NULL_VERSION' => 'FALSE'}

        if nullLckHandle
            kvp_hash['NULL_LCK_HANDLE'] = 'TRUE'
        else
            kvp_hash['NULL_LCK_HANDLE'] = 'FALSE'
            kvp_hash['LOCK_WAITER_CB'] = 'TRUE'
            kvp_hash['RESOURCE_UNLOCK_CB'] = 'TRUE'
            kvp_hash['LOCK_GRANT_CB'] = 'TRUE'
        end

        if nullCallbacks
            kvp_hash['NULL_CALLBACKS'] = 'TRUE'
        end
        if nullVersion
            kvp_hash['NULL_VERSION'] = 'TRUE'
        end
        runDriver("INITIALIZE_REQ", kvp_hash, expectedReturn)
    end

    def finalize(resourceID, expectedReturn)
        kvp_hash = {'LCK_RESOURCE_ID' => resourceID}
        runDriver("FINALIZE_REQ", kvp_hash, expectedReturn)
    end

    def selectObjectGet(resourceID, nullSelectionObject, expectedReturn)
        kvp_hash = {'LCK_RESOURCE_ID' => resourceID,
                    'NULL_SELECTION_OBJECT' => 'FALSE'}
        if nullSelectionObject
            kvp_hash['NULL_SELECTION_OBJECT'] = 'TRUE'
        end
        runDriver("SELECTION_OBJECT_GET_REQ", kvp_hash, expectedReturn)

    end

    def dispatch(resourceID, dispatchFlags, expectedReturn)
        kvp_hash = {'LCK_RESOURCE_ID' => resourceID,
                    'DISPATCH_FLAGS' => dispatchFlags}
        runDriver("DISPATCH_REQ", kvp_hash, expectedReturn)
    end

    def resourceOpen(resourceID, lockName, expectedReturn)
        kvp_hash = {'LCK_RESOURCE_ID' => resourceID,
                    'LOCK_NAME' => lockName}
        runDriver("LOCK_RESOURCE_OPEN_REQ", kvp_hash, expectedReturn)
    end

    def resourceClose(resourceID, expectedReturn)
        kvp_hash = {'LCK_RESOURCE_ID' => resourceID}
        runDriver("LOCK_RESOURCE_CLOSE_REQ", kvp_hash, expectedReturn)
    end

    def lockSync(resourceID, lockMode, waiterSignal, nullLockID, nullStatus, 
                 noQueueFlag, orphanFlag, invalidFlag, 
                 expectedStatus, expectedReturn)
        kvp_hash = {'LCK_RESOURCE_ID' => resourceID,
                    'LOCK_MODE' => lockMode,
                    'WAITER_SIGNAL' => waiterSignal,
                    'NULL_LOCK_ID' => 'FALSE',
                    'NULL_LOCK_STATUS' => 'FALSE',
                    'LOCK_FLAG_NO_QUEUE' => 'FALSE',
                    'LOCK_FLAG_ORPHAN' => 'FALSE',
                    'LOCK_FLAG_INVALID' => 'FALSE',
                    'EXPECTED_STATUS' => expectedStatus}
        if (true == noQueueFlag)
            kvp['LOCK_FLAG_NO_QUEUE'] = 'TRUE'
        end
        if (true == orphanFlag)
            kvp['LOCK_FLAG_ORPHAN'] = 'TRUE'
        end
        if (true == invalidFlag)
            kvp['LOCK_FLAG_INVALID'] = 'TRUE'
        end
        if (true == nullLockID)
            kvp['NULL_LOCK_ID'] = 'TRUE'
        end
        if (true == nullStatus)
            kvp['NULL_LOCK_STATUS'] = 'TRUE'
        end
        runDriver("LOCK_SYNC_REQ", kvp_hash, expectedReturn)
    end

    def lockAsync(resourceID, lockMode, invocation, waiterSignal, 
                  nullLockID, noQueueFlag, orphanFlag, invalidFlag, 
                  expectedReturn)
        kvp_hash = {'LCK_RESOURCE_ID' => resourceID,
                    'LOCK_MODE' => lockMode,
                    'INVOCATION' => invocation,
                    'WAITER_SIGNAL' => waiterSignal,
                    'NULL_LOCK_ID' => 'FALSE',
                    'LOCK_FLAG_NO_QUEUE' => 'FALSE',
                    'LOCK_FLAG_ORPHAN' => 'FALSE',
                    'LOCK_FLAG_INVALID' => 'FALSE'}
        if (true == noQueueFlag)
            kvp['LOCK_FLAG_NO_QUEUE'] = 'TRUE'
        end
        if (true == orphanFlag)
            kvp['LOCK_FLAG_ORPHAN'] = 'TRUE'
        end
        if (true == invalidFlag)
            kvp['LOCK_FLAG_INVALID'] = 'TRUE'
        end
        if (true == nullLockID)
            kvp['NULL_LOCK_ID'] = 'TRUE'
        end
        runDriver("LOCK_ASYNC_REQ", kvp_hash, expectedReturn)
    end

    def unlockSync(resourceID, expectedReturn)
        kvp_hash = {'LCK_RESOURCE_ID' => resourceID}
        runDriver("UNLOCK_SYNC_REQ", kvp_hash, expectedReturn)
    end

    def unlockAsync(resourceID, invocation, expectedReturn)
        kvp_hash = {'LCK_RESOURCE_ID' => resourceID,
                    'INVOCATION' => invocation}
        runDriver("UNLOCK_ASYNC_REQ", kvp_hash, expectedReturn)
    end

    def lockGetWaitCount(resourceID, expectedLastWaiterSignal,
                         expectedNotificationCount)
        kvp_hash = {'LCK_RESOURCE_ID' => resourceID}
        array = runDriver("LOCK_GET_WAIT_COUNT_REQ", kvp_hash, expectedReturn)
        ret = array[0]
        lines = array[1]
        lastWaiterSignal = nil
        waiterNotifyCount = nil
        lines.each do |line|
            if line =~ /^Last Delivered Lock Waiter Signal=(\d+)/
                lastWaiterSignal = $1
            elsif line =~ /^Lock Waiter Notification Count=(\d+)/
                waiterNotifyCount = $1
            end
        end
        if nil == lastWaiterSignal
            raise "Couldnt find a Lock Waiter Signal"
        end
        if nil == waiterNotifyCount
            raise "Couldnt find a Lock Waiter Notification Count"
        end
        if lastWaiterSignal != expectedLastWaiterSignal.to_s
            raise "Expected waiter signal %s, got %s.  Lines = \"%s\"" % \
                  [expectedLastWaiterSignal, lastWaiterSignal, lines.to_s]
        end
        if waiterNotifyCount != expectedNotificationCount.to_s
            raise "Expected notify count %s, got %s.  Lines = \"%s\"" % \
                  [expectedNotificationCount, waiterNotifyCount, lines.to_s]
        end
    end

    def lockGetAsyncLockStatus(resourceID, expectedInvocation, expectedStatus,
                               expectedReturn)
        kvp_hash = {'LCK_RESOURCE_ID' => resourceID}
        array = runDriver("LOCK_GET_ASYNC_LOCK_STATUS_REQ", 
                          kvp_hash, expectedReturn)
        ret = array[0]
        lines = array[1]
        invocation = nil
        status = nil
        lines.each do |line|
            if line =~ /^Async Lock Invocation=(\d+)/
                invocation = $1
            elsif line =~ /^Async Lock Status=(\S+)/
                status = $1
            end
        end
        if nil == invocation
            raise "Couldnt find an invocation.  Lines = \"%s\"" % \
                  [lines.to_s]
        end
        if nil == status
            raise "Couldnt find a status.  Lines = \"%s\"" % \
                  [lines.to_s]
        end
        if invocation != expectedInvocation.to_s
            raise "Expected invocation %s, got %s.  Lines = \"%s\"" % \
                  [expectedInvocation.to_s, invocation.to_s, lines.to_s]
        end
        if status != expectedStatus.to_s
            raise "Expected status %s, got %s.  Lines = \"%s\"" % \
                  [expectedStatus.to_s, status.to_s, lines.to_s]
        end
        if expectedReturn != ret
            raise "Expected return %s, got %s.  Lines = \"%s\"" % \
                   [mapErrorCodeToString(expectedReturn), 
                    mapErrorCodeToString(ret), lines.to_s]
        end
    end

    def lockGetAsyncUnlockStatus(resourceID, expectedInvocation, expectedStatus,
                                 expectedReturn)
        kvp_hash = {'LCK_RESOURCE_ID' => resourceID}
        array = runDriver("LOCK_GET_ASYNC_UNLOCK_STATUS_REQ", 
                          kvp_hash, expectedReturn)
        ret = array[0]
        lines = array[1]
        invocation = nil
        status = nil
        lines.each do |line|
            if line =~ /^Async Unlock Invocation=(\d+)/
                invocation = $1
            elsif line =~ /^Async Unlock Status=(\S+)/
                status = $1
            end
        end
        if nil == invocation
            raise "Couldnt find an invocation.  Lines = \"%s\"" % \
                  [lines.to_s]
        end
        if nil == status
            raise "Couldnt find a status.  Lines = \"%s\"" % \
                  [lines.to_s]
        end
        if invocation != expectedInvocation.to_s
            raise "Expected invocation %s, got %s.  Lines = \"%s\"" % \
                  [expectedInvocation.to_s, invocation.to_s, lines.to_s]
        end
        if status != expectedStatus.to_s
            raise "Expected status %s, got %s.  Lines = \"%s\"" % \
                  [expectedStatus.to_s, status.to_s, lines.to_s]
        end
        if expectedReturn != ret
            raise "Expected return %s, got %s.  Lines = \"%s\"" % \
                   [mapErrorCodeToString(expectedReturn), 
                    mapErrorCodeToString(ret), lines.to_s]
        end
    end
end # class

end # module
