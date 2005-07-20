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

    def createTestResource()
        cmd = "%s --run-dir %s --o CREATE_TEST_RES --socket-file %s --load-libs %s" % \
              [getDriverPath(), getRunDir(), getSocketFile(), getDriverLibs()]
        array = captureCommand(cmd)
        ret = array[0]
        lines = array[1]
        resourceID = nil
        lines.each do |line|
            if line =~ /^Resource ID=(\d+)/
                resourceID = $1
            end
        end
        if nil == resourceID 
            raise "Couldn't find a Resource ID"
        end
        log("new resourceID is %d" % [resourceID])
        return resourceID
    end

    def runDriver(cmd, expectedReturn)
        newCmd = "%s --run-dir %s --socket-file %s --load-libs %s %s" % \
                 [getDriverPath(), getRunDir(), getSocketFile(),
                  getDriverLibs(), cmd]
        array = captureCommand(newCmd)
        ret = array[0]
        lines = array[1]
        if expectedReturn != ret
            raise "Expected return %s, got %s.  Lines = \"%s\"" % \
                   [mapErrorCodeToString(expectedReturn), 
                    mapErrorCodeToString(ret), lines.to_s]
        end
    end

    def init(resourceID, dispatchFlags, expectedReturn)
        cmd = "--o INIT --resource-id %s --set-lock-grant-cb --set-resource-unlock-cb --set-lock-waiter-cb --dispatch-flags %s" % [resourceID, dispatchFlags]
        runDriver(cmd, expectedReturn)
    end

    def initWithOptions(resourceID, dispatchFlags, 
                        releaseCode, majorVersion, minorVersion,
                        nullLckHandle, nullCallbacks, nullVersion,
                        expectedReturn)
        cmd = "--o INIT --resource-id %s --dispatch-flags %s --version-release-code %d --version-major %d --version-minor %d" % \
              [resourceID, dispatchFlags, 
               releaseCode, majorVersion, minorVersion]
        if nullLckHandle
            cmd += " --null-lck-handle"
        end
        if nullCallbacks
            cmd += " --null-callbacks"
        end
        if nullVersion
            cmd += " --null-version"
        end
        runDriver(cmd, expectedReturn)
    end

    def finalize(resourceID, expectedReturn)
        cmd = "--o FINALIZE --resource-id %s" % [resourceID]
        runDriver(cmd, expectedReturn)
    end

    def selectObjectGet(resourceID, nullSelectionObject, expectedReturn)
        cmd = "--o SELECT_OBJ_GET --resource-id %s" % [resourceID]
        if nullSelectionObject
            cmd += " --null-selection-object"
        end
        runDriver(cmd, expectedReturn)
    end

    def resourceOpen(resourceID, lockName, expectedReturn)
        cmd = "--o RES_OPEN --resource-id %s --lock-name %s" \
              % [resourceID, lockName]
        runDriver(cmd, expectedReturn)
    end

    def resourceClose(resourceID, expectedReturn)
        cmd = "--o RES_CLOSE --resource-id %d" % [resourceID]
        runDriver(cmd, expectedReturn)
    end

    def dispatch(resourceID, dispatchFlags, expectedReturn)
        cmd = "--o DISPATCH --resource-id %s --dispatch-flags %s" % \
              [resourceID, dispatchFlags]
        runDriver(cmd, expectedReturn)
    end

    def lockSync(resourceID, lockMode, waiterSignal, nullLockID, nullStatus, 
                 noQueueFlag, orphanFlag, invalidFlag, 
                 expectedStatus, expectedReturn)
        cmd = "--o LOCK_SYNC --resource-id %s --lock-mode %s --waiter-signal %d --expected-status %s" % \
              [resourceID, lockMode, waiterSignal, expectedStatus]
        if (true == noQueueFlag)
            cmd += " --lock-flag-no-queue"
        end
        if (true == orphanFlag)
            cmd += " --lock-flag-orphan"
        end
        if (true == invalidFlag)
            cmd += " --lock-flag-invalid"
        end
        if (true == nullLockID)
            cmd += " --null-lock-id"
        end
        if (true == nullStatus)
            cmd += " --null-lock-status"
        end
        runDriver(cmd, expectedReturn)
    end

    def lockAsync(resourceID, lockMode, invocation, waiterSignal, 
                  nullLockID, noQueueFlag, orphanFlag, invalidFlag, 
                  expectedReturn)
        cmd = "--o LOCK_ASYNC --resource-id %s --lock-mode %s --invocation %d --waiter-signal %d" % \
              [resourceID, lockMode, invocation, waiterSignal]
        if (true == noQueueFlag)
            cmd += " --lock-flag-no-queue"
        end
        if (true == orphanFlag)
            cmd += " --lock-flag-orphan"
        end
        if (true == invalidFlag)
            cmd += " --lock-flag-invalid"
        end
        if (true == nullLockID)
            cmd += " --null-lock-id"
        end
        runDriver(cmd, expectedReturn)
    end

    def unlockSync(resourceID, expectedReturn)
        cmd = "--o UNLOCK_SYNC --resource-id %s" % [resourceID]
        runDriver(cmd, expectedReturn)
    end

    def unlockAsync(resourceID, invocation, expectedReturn)
        cmd = "--o UNLOCK_ASYNC --resource-id %s --invocation %d" % \
              [resourceID, invocation]
        runDriver(cmd, expectedReturn)
    end

    def lockGetWaitCount(resourceID, expectedLastWaiterSignal,
                         expectedNotificationCount)
        cmd = "%s --run-dir %s --socket-file %s --load-libs %s --o WAIT_COUNT --resource-id %s" % \
              [getDriverPath(), getRunDir(), getSocketFile(), getDriverLibs(),
               resourceID]
        array = captureCommand(cmd)
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
        cmd = "%s --run-dir %s --socket-file %s --load-libs %s --o ASYNC_LOCK_STATUS --resource-id %s" % \
              [getDriverPath(), getRunDir(), getSocketFile(), 
               getDriverLibs(), resourceID]
        array = captureCommand(cmd)
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
        cmd = "%s --run-dir %s --socket-file %s --load-libs %s --o ASYNC_UNLOCK_STATUS --resource-id %s" % \
              [getDriverPath(), getRunDir(), getSocketFile(), 
               getDriverLibs(), resourceID]
        array = captureCommand(cmd)
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
