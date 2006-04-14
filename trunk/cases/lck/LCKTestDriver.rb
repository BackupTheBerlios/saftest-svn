module SAFTest

require 'SAFTestDriver'
require 'SAFTestUtils'
require 'SAFTestAction'

class LCKTestResource < SAFTestUtils
    def initialize(resourceID)
        @resourceID = resourceID
        @longLived = false
        @dispatchFlags = ''
    end

    def getID()
        return @resourceID
    end

    def getDispatchFlags()
        return @dispatchFlags
    end

    def setDispatchFlags(dispatchFlags)
        @dispatchFlags = dispatchFlags
    end

    def isLongLived()
        return @longLived
    end

    def setLongLived(longLived)
        @longLived = longLived
    end
end

class LCKTestDriver < SAFTestDriver
    @@nextInstanceID = 1

    def initialize(node=nil, instanceID=0)
        super(node, nil, instanceID)
    end

private

    def createTestResource(longLived)
        kvpHash = {"LCK_RESOURCE_LONG_LIVED" => "FALSE"}
        if longLived == true
            kvpHash["LCK_RESOURCE_LONG_LIVED"] = "TRUE"
        end
        results = runDriver("CREATE_TEST_RESOURCE_REQ", kvpHash,
                                SAFTestUtils.SA_AIS_OK)
        resourceID = results["LCK_RESOURCE_ID"].to_i
        resource = LCKTestResource.new(resourceID)
        return resource
    end

public

    def getName()
        return 'lck_driver'
    end

    def waitForLockAsyncLockStatus(resource, expectedInvocation,
                                   expectedStatus, expectedReturn)
        waitFor(@@WAIT_INTERVAL, @@WAIT_TIMEOUT,
                "async lock status to be invocation=#{expectedInvocation}" + 
                "status=#{expectedStatus}, return=#{expectedReturn}") {
            kvpHash = {'LCK_RESOURCE_ID' => resource.getID()}
            kvpResults = runDriver("LOCK_GET_ASYNC_LOCK_STATUS_REQ", 
                                   kvpHash, SAFTestUtils.SA_AIS_OK)
            invocation = kvpResults['ASYNC_LOCK_INVOCATION'].to_i
            status = kvpResults['LOCK_STATUS']
            errorStatus = kvpResults['ASYNC_LOCK_ERROR_STATUS'].to_i
            log("async lock status: invocation=#{invocation} " + 
                "status=#{status}, return=#{errorStatus}")
            invocation == expectedInvocation and \
                status == expectedStatus and \
                errorStatus == expectedReturn
        }
    end

    def waitForLockAsyncUnlockStatus(resource, expectedInvocation, 
                                     expectedStatus, expectedReturn)
        waitFor(@@WAIT_INTERVAL, @@WAIT_TIMEOUT,
                "async unlock status to be invocation=#{expectedInvocation} " + 
                "status=#{expectedStatus}, return=#{expectedReturn}") {
            kvpHash = {'LCK_RESOURCE_ID' => resource.getID()}
            kvpResults = runDriver("LOCK_GET_ASYNC_UNLOCK_STATUS_REQ", 
                                   kvpHash, SAFTestUtils.SA_AIS_OK)
            invocation = kvpResults['ASYNC_UNLOCK_INVOCATION'].to_i
            status = kvpResults['LOCK_STATUS']
            errorStatus = kvpResults['ASYNC_UNLOCK_ERROR_STATUS'].to_i
            log("async unlock status: invocation=#{invocation} " + 
                "status=#{status}, return=#{errorStatus}")
            invocation == expectedInvocation and \
                status == expectedStatus and \
                errorStatus == expectedReturn
        }
    end

    def waitForLockSyncBG(action, expectedReturn)
        waitFor(@@WAIT_INTERVAL, @@WAIT_TIMEOUT,
                "lock sync background action to finish") {
            action.load()
            action.stopTime != nil
        }
        
        # There is only 1 command in these actions, id 0
        cmd = action.getCommand("0")
        cmd.load()
        ret = cmd.result.to_i
        lines = cmd.output()
        if expectedReturn != ret
            raise "Expected return %s, got %s.  Lines = \"%s\"" % \
                   [mapErrorCodeToString(expectedReturn),
                    mapErrorCodeToString(ret), lines.to_s]
        end
    end

    def createLongLivedTestResource()
        createTestResource(true)
    end

    def createShortLivedTestResource()
        createTestResource(false)
    end

    def deleteTestResource(resource)
        kvpHash = {'LCK_RESOURCE_ID' => resource.getID()}
        results = runDriver("DELETE_TEST_RESOURCE_REQ", kvpHash,
                            SAFTestUtils.SA_AIS_OK)
    end

    def LCKTestDriver.getLongLivedDrivers(node)
        utils = SAFTestUtils.new
        config = SAFTestConfig.new
        config.loadFromXML(utils.configXMLFile())

        numDrivers = config.getIntValue('main', 'numLongLivedDrivers')
        driverArray = []
        1.upto(numDrivers) do |n|
            driver = LCKTestDriver.new(node, n)
            driverArray << driver
        end
        return driverArray
    end

    def LCKTestDriver.getRandomLongLivedDriver(node)
        driverArray = LCKTestDriver.getLongLivedDrivers(node)
        return driverArray[0]
    end

    def LCKTestDriver.startNewShortLivedDriver(node)
        driver = SAFTestDriver.startNewShortLivedDriver(node)
        return LCKTestDriver.new(node, driver.getInstanceID())
    end

    def getAllTestResources()
        resourceArray = []
        results = runDriver("STATUS_REQ", {},
                            SAFTestUtils.SA_AIS_OK)
        numResources = results["NUM_LCK_RESOURCES"].to_i
        0.upto(numResources - 1) do |n|
            resourceID = results["LCK_RESOURCE_#{n}_ID"]
            resource = LCKTestResource.new(resourceID)
            dispatchFlags = results["LCK_RESOURCE_#{n}_DISPATCH_FLAGS"]
            resource.setDispatchFlags(dispatchFlags)
            isLongLived = results["LCK_RESOURCE_#{n}_LONG_LIVED"]
            if isLongLived == "TRUE"
                resource.setLongLived(true)
            else
                resource.setLongLived(false)
            end
            resourceArray << resource
        end
        return resourceArray
    end

    def getAllLongLivedTestResources()
        resourceArray = getAllTestResources()
        longLivedArray = []
        resourceArray.each do |r|
            if r.isLongLived()
                longLivedArray << r
            end
        end
        return resourceArray
    end

    def getRandomLongLivedTestResource()
        longLivedArray = getAllLongLivedTestResources()
        return longLivedArray[rand(longLivedArray.length())]
    end

    # The Main test resource is the first long lived one.  This is the
    # resource that can call saClmClusterTrack with the TRACK_CURRENT flag
    # and expect the results via a callback.
    def getMainLongLivedTestResource()
        longLivedArray = getAllLongLivedTestResources()
        return longLivedArray[0]
    end

    def init(resource, setResourceOpenCB, setLockGrantCB, setLockWaiterCB,
             setResourceUnlockCB, dispatchFlags, expectedReturn)
        kvpHash = {'LCK_RESOURCE_ID' => resource.getID(),
                   'DISPATCH_FLAGS' => dispatchFlags,
                   'RESOURCE_OPEN_CB' => 'FALSE',
                   'LOCK_GRANT_CB' => 'FALSE',
                   'LOCK_WAITER_CB' => 'FALSE',
                   'RESOURCE_UNLOCK_CB' => 'FALSE',
                   'VERSION_RELEASE_CODE' => SAFTestUtils.SA_AIS_RELEASE_CODE,
                   'VERSION_MAJOR' => SAFTestUtils.SA_AIS_MAJOR_VERSION,
                   'VERSION_MINOR' => SAFTestUtils.SA_AIS_MINOR_VERSION,
                   'DISPATCH_FLAGS' => dispatchFlags,
                   'NULL_LCK_HANDLE' => 'FALSE',
                   'NULL_CALLBACKS' => 'FALSE',
                   'NULL_VERSION' => 'FALSE'}

        if setResourceOpenCB
            kvpHash['RESOURCE_OPEN_CB'] = 'TRUE'
        end
        if setLockGrantCB
            kvpHash['LOCK_GRANT_CB'] = 'TRUE'
        end
        if setLockWaiterCB
            kvpHash['LOCK_WAITER_CB'] = 'TRUE'
        end
        if setResourceUnlockCB
            kvpHash['RESOURCE_UNLOCK_CB'] = 'TRUE'
        end
        runDriver("INITIALIZE_REQ", kvpHash, expectedReturn)
    end

    def initWithOptions(resource, dispatchFlags, 
                        releaseCode, majorVersion, minorVersion,
                        nullLckHandle, nullCallbacks, nullVersion,
                        expectedReturn)
        kvpHash = {'LCK_RESOURCE_ID' => resource.getID(),
                   'VERSION_RELEASE_CODE' => releaseCode,
                   'VERSION_MAJOR' => majorVersion,
                   'VERSION_MINOR' => minorVersion,
                   'NULL_LCK_HANDLE' => 'FALSE',
                   'NULL_VERSION' => 'FALSE',
                   'DISPATCH_FLAGS' => dispatchFlags}

        if nullLckHandle
            kvpHash['NULL_LCK_HANDLE'] = 'TRUE'
        end

        if nullCallbacks
            kvpHash['NULL_CALLBACKS'] = 'TRUE'
        else
            kvpHash['NULL_CALLBACKS'] = 'FALSE'
            kvpHash['RESOURCE_OPEN_CB'] = 'TRUE'
            kvpHash['LOCK_GRANT_CB'] = 'TRUE'
            kvpHash['LOCK_WAITER_CB'] = 'TRUE'
            kvpHash['RESOURCE_UNLOCK_CB'] = 'TRUE'
        end

        if nullVersion
            kvpHash['NULL_VERSION'] = 'TRUE'
        end
        runDriver("INITIALIZE_REQ", kvpHash, expectedReturn)
    end

    def finalize(resource, expectedReturn)
        kvpHash = {'LCK_RESOURCE_ID' => resource.getID()}
        runDriver("FINALIZE_REQ", kvpHash, expectedReturn)
    end

    def selectObjectGet(resource, nullSelectionObject, expectedReturn)
        kvpHash = {'LCK_RESOURCE_ID' => resource.getID(),
                   'NULL_SELECTION_OBJECT' => 'FALSE'}
        if nullSelectionObject
            kvpHash['NULL_SELECTION_OBJECT'] = 'TRUE'
        end
        runDriver("SELECTION_OBJECT_GET_REQ", kvpHash, expectedReturn)

    end

    def dispatch(resource, dispatchFlags, expectedReturn)
        kvpHash = {'LCK_RESOURCE_ID' => resource.getID(),
                   'DISPATCH_FLAGS' => dispatchFlags}
        runDriver("DISPATCH_REQ", kvpHash, expectedReturn)
    end

    def resourceOpen(resource, lockName, expectedReturn)
        kvpHash = {'LCK_RESOURCE_ID' => resource.getID(),
                   'LOCK_NAME' => lockName}
        runDriver("LOCK_RESOURCE_OPEN_REQ", kvpHash, expectedReturn)
    end

    def resourceClose(resource, expectedReturn)
        kvpHash = {'LCK_RESOURCE_ID' => resource.getID()}
        runDriver("LOCK_RESOURCE_CLOSE_REQ", kvpHash, expectedReturn)
    end

private

    def getLockSyncKVPHash(resource, lockMode, waiterSignal, nullLockID, 
                           nullStatus, noQueueFlag, orphanFlag, invalidFlag,
                           expectedStatus)
        kvpHash = {'LCK_RESOURCE_ID' => resource.getID(),
                    'LOCK_MODE' => lockMode,
                    'WAITER_SIGNAL' => waiterSignal,
                    'NULL_LOCK_ID' => 'FALSE',
                    'NULL_LOCK_STATUS' => 'FALSE',
                    'LOCK_FLAG_NO_QUEUE' => 'FALSE',
                    'LOCK_FLAG_ORPHAN' => 'FALSE',
                    'LOCK_FLAG_INVALID' => 'FALSE',
                    'EXPECTED_LOCK_STATUS' => expectedStatus}
        if (true == noQueueFlag)
            kvpHash['LOCK_FLAG_NO_QUEUE'] = 'TRUE'
        end
        if (true == orphanFlag)
            kvpHash['LOCK_FLAG_ORPHAN'] = 'TRUE'
        end
        if (true == invalidFlag)
            kvpHash['LOCK_FLAG_INVALID'] = 'TRUE'
        end
        if (true == nullLockID)
            kvpHash['NULL_LOCK_ID'] = 'TRUE'
        end
        if (true == nullStatus)
            kvpHash['NULL_LOCK_STATUS'] = 'TRUE'
        end
        return kvpHash
    end

public

    def lockSync(resource, lockMode, waiterSignal, nullLockID, nullStatus, 
                 noQueueFlag, orphanFlag, invalidFlag, 
                 expectedStatus, expectedReturn)
        kvpHash = getLockSyncKVPHash(resource, lockMode, waiterSignal,
                                     nullLockID, nullStatus, noQueueFlag, 
                                     orphanFlag, invalidFlag, expectedStatus)
        runDriver("LOCK_SYNC_REQ", kvpHash, expectedReturn)
    end

    def lockSyncBG(resource, lockMode, waiterSignal, nullLockID, nullStatus, 
                   noQueueFlag, orphanFlag, invalidFlag, 
                   expectedStatus)
        kvpHash = getLockSyncKVPHash(resource, lockMode, waiterSignal,
                                     nullLockID, nullStatus, noQueueFlag, 
                                     orphanFlag, invalidFlag, expectedStatus)
        return runDriverBG("LOCK_SYNC_REQ", kvpHash)
    end

    def lockAsync(resource, lockMode, invocation, waiterSignal, 
                  nullLockID, noQueueFlag, orphanFlag, invalidFlag, 
                  expectedReturn)
        kvpHash = {'LCK_RESOURCE_ID' => resource.getID(),
                   'LOCK_MODE' => lockMode,
                   'INVOCATION' => invocation,
                   'WAITER_SIGNAL' => waiterSignal,
                   'NULL_LOCK_ID' => 'FALSE',
                   'LOCK_FLAG_NO_QUEUE' => 'FALSE',
                   'LOCK_FLAG_ORPHAN' => 'FALSE',
                   'LOCK_FLAG_INVALID' => 'FALSE'}
        if (true == noQueueFlag)
            kvpHash['LOCK_FLAG_NO_QUEUE'] = 'TRUE'
        end
        if (true == orphanFlag)
            kvpHash['LOCK_FLAG_ORPHAN'] = 'TRUE'
        end
        if (true == invalidFlag)
            kvpHash['LOCK_FLAG_INVALID'] = 'TRUE'
        end
        if (true == nullLockID)
            kvpHash['NULL_LOCK_ID'] = 'TRUE'
        end
        runDriver("LOCK_ASYNC_REQ", kvpHash, expectedReturn)
    end

    def unlockSync(resource, expectedReturn)
        kvpHash = {'LCK_RESOURCE_ID' => resource.getID()}
        runDriver("UNLOCK_SYNC_REQ", kvpHash, expectedReturn)
    end

    def unlockAsync(resource, invocation, expectedReturn)
        kvpHash = {'LCK_RESOURCE_ID' => resource.getID(),
                   'INVOCATION' => invocation}
        runDriver("UNLOCK_ASYNC_REQ", kvpHash, expectedReturn)
    end

    def lockGetWaitCount(resource, expectedLastWaiterSignal,
                         expectedNotificationCount)
        kvpHash = {'LCK_RESOURCE_ID' => resource.getID()}
        kvpResults = runDriver("LOCK_GET_WAIT_COUNT_REQ", kvpHash, 
                               SAFTestUtils.SA_AIS_OK)
        lastWaiterSignal = kvpResults['LAST_DELIVERED_WAITER_SIGNAL'].to_i
        waiterNotifyCount = kvpResults['WAITER_SIGNAL_NOTIFICATION_COUNT'].to_i
        if lastWaiterSignal != expectedLastWaiterSignal
            raise "Expected waiter signal %d, got %d." % \
                  [expectedLastWaiterSignal, lastWaiterSignal]
        end
        if waiterNotifyCount != expectedNotificationCount
            raise "Expected notify count %d, got %d." % \
                  [expectedNotificationCount, waiterNotifyCount]
        end
    end
end # class

end # module
