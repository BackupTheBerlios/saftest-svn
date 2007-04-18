module SAFTest

require 'SAFTestDriver'
require 'SAFTestUtils'
require 'SAFTestAction'

class LCKSession < SAFTestUtils
    def initialize(sessionID)
        @sessionID = sessionID
        @longLived = false
        @global = false
        @dispatchFlags = ''
    end

    def getID()
        return @sessionID
    end

    def getDispatchFlags()
        return @dispatchFlags
    end

    def setDispatchFlags(dispatchFlags)
        @dispatchFlags = dispatchFlags
    end

    def longLived?()
        return @longLived
    end

    def setLongLived(longLived)
        @longLived = longLived
    end

    def global?()
        return @global
    end

    def setGlobal(global)
        @global = global
    end
end

class LCKTestDriver < SAFTestDriver
    def initialize(params = {})
        super(params)
    end

private

    def createSession(params = {})
        kvpHash = {"LCK_SESSION_LONG_LIVED" => "FALSE",
                   "LCK_SESSION_GLOBAL" => "FALSE"}
        if params[:LONG_LIVED]
            kvpHash["LCK_SESSION_LONG_LIVED"] = "TRUE"
        end
        if params[:GLOBAL]
            kvpHash["LCK_SESSION_GLOBAL"] = "TRUE"
        end
        results = runDriver(params.update({:OP => "CREATE_SESSION_REQ",
                                           :KVP_HASH => kvpHash}))
        sessionID = results["LCK_SESSION_ID"].to_i
        session = LCKSession.new(sessionID)
        return session
    end

public

    # This needs to be public in order to be called from the base class
    def initializeResources()
        threads = getThreads(:SPEC => "LCK")
        if multiThreaded?()
            session = createLongLivedSession(:GLOBAL => true)
            init(:SESSION => session,
                 :DISPATCH_FLAGS => "SA_DISPATCH_BLOCKING")

            threads.each do |thread|
                if thread.dispatchThread?()
                    dispatch(:SESSION => session,
                             :THREAD => thread,
                             :DISPATCH_FLAGS => "SA_DISPATCH_BLOCKING")
                end
            end

        else
            session = createLongLivedSession()
            init(:SESSION => session,
                 :DISPATCH_FLAGS => "SA_DISPATCH_ALL")
            selectObjectGet(:SESSION => session)

            session = createLongLivedSession()
            init(:SESSION => session,
                 :DISPATCH_FLAGS => "SA_DISPATCH_ONE")
            selectObjectGet(:SESSION => session)

            # We'll create one session that doesn't do automatic dispatching
            session = createLongLivedSession()
            init(:SESSION => session,
                 :DISPATCH_FLAGS => "SAFTEST_DISPATCH_NONE")
        end
    end

    def getName()
        return 'lck_driver'
    end
   
    def waitForWaiterNotificationCount(params = {})
        expectedLastWaiterSignal = params[:EXPECTED_LAST_WAITER_SIGNAL]
        expectedNotificationCount = params[:EXPECTED_NOTIFICATION_COUNT]
         waitFor(@@WAIT_INTERVAL, @@WAIT_TIMEOUT,
                 "waiter notification count to be " + \
                 "signal=#{expectedLastWaiterSignal} " + \
                 "notification count=#{expectedNotificationCount}") {
            kvpHash = {'LCK_SESSION_ID' => params[:SESSION].getID()}
            kvpResults = runDriver(params.update(
                {:OP => "LOCK_GET_WAIT_COUNT_REQ", 
                 :KVP_HASH => kvpHash}))
            lastWaiterSignal = kvpResults['LAST_DELIVERED_WAITER_SIGNAL'].to_i
            waiterNotifyCount = kvpResults['WAITER_SIGNAL_NOTIFICATION_COUNT'].to_i
             log("waiter notification status: " +
                    "signal=#{lastWaiterSignal}, count=#{waiterNotifyCount}")
             lastWaiterSignal == expectedLastWaiterSignal and \
                 waiterNotifyCount == expectedNotificationCount
        }
    end

    def waitForResourceOpenAsyncStatus(params = {})
        expectedReturn = getExpectedReturnFromParams(params)
        expectedInvocation = params[:EXPECTED_INVOCATION]
         waitFor(@@WAIT_INTERVAL, @@WAIT_TIMEOUT,
                 "async resource open status to be invocation=#{expectedInvocation}" +
                  "return=#{expectedReturn}") {
             kvpHash = {'LCK_SESSION_ID' => params[:SESSION].getID()}
             kvpResults = runDriver(params.update(
                {:OP => "LOCK_GET_RESOURCE_OPEN_ASYNC_STATUS_REQ",
                 :KVP_HASH => kvpHash}))

             invocation = kvpResults['ASYNC_OPEN_INVOCATION'].to_i
             errorStatus = kvpResults['ASYNC_OPEN_ERROR_STATUS'].to_i
             log("async open status: " +
                    "invocation=#{invocation}, return=#{errorStatus}")
             invocation == expectedInvocation and errorStatus == expectedReturn
        }
    end

    def waitForLockAsyncLockStatus(params = {})
        expectedReturn = getExpectedReturnFromParams(params)
        expectedInvocation = params[:EXPECTED_INVOCATION]
        expectedStatus = params[:EXPECTED_LOCK_STATUS]
        waitFor(@@WAIT_INTERVAL, @@WAIT_TIMEOUT,
                "async lock status to be invocation=#{expectedInvocation} " + 
                "status=#{expectedStatus}, return=#{expectedReturn}") {
            kvpHash = {'LCK_SESSION_ID' => params[:SESSION].getID()}
            kvpResults = runDriver(params.update(
                {:OP => "LOCK_GET_ASYNC_LOCK_STATUS_REQ", 
                 :KVP_HASH => kvpHash}))
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

    def waitForLockAsyncUnlockStatus(params = {})
        expectedReturn = getExpectedReturnFromParams(params)
        expectedInvocation = params[:EXPECTED_INVOCATION]
        expectedStatus = params[:EXPECTED_LOCK_STATUS]
        waitFor(@@WAIT_INTERVAL, @@WAIT_TIMEOUT,
                "async unlock status to be invocation=#{expectedInvocation} " + 
                "status=#{expectedStatus}, return=#{expectedReturn}") {
            kvpHash = {'LCK_SESSION_ID' => params[:SESSION].getID()}
            kvpResults = runDriver(params.update(
                {:OP => "LOCK_GET_ASYNC_UNLOCK_STATUS_REQ", 
                 :KVP_HASH => kvpHash}))
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

    def waitForLockSyncBG(params = {})
        action = params[:ACTION]
        expectedReturn = getExpectedReturnFromParams(params)
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

    def createLongLivedSession(params = {})
        params[:LONG_LIVED] = true
        return createSession(params)
    end

    def createShortLivedSession(params = {})
        params[:LONG_LIVED] = false
        return createSession(params)
    end

    def deleteSession(params = {})
        kvpHash = {'LCK_SESSION_ID' => params[:SESSION].getID()}
        results = runDriver(params.update({:OP => "DELETE_SESSION_REQ", 
                                           :KVP_HASH => kvpHash}))
    end

    def LCKTestDriver.getLongLivedDrivers(params = {})
        utils = SAFTestUtils.new
        config = SAFTestConfig.new
        config.loadFromXML(utils.configXMLFile())

        node = nil
        if params[:NODE]
            node = params[:NODE]
        end
        numDrivers = config.getIntValue('main', 'numLongLivedDrivers')
        driverArray = []
        1.upto(numDrivers) do |n|
            driver = LCKTestDriver.new(:NODE => node, :INSTANCE_ID => n)

            # Test cases get single-threaded drivers unless they specifically
            # ask for a multi-threaded driver
            if (not params[:THREADED] and not driver.multiThreaded?()) or
                (params[:THREADED] and driver.multiThreaded?())
                exclude = false
                if params[:EXCLUDE_DRIVER_SET]
                    params[:EXCLUDE_DRIVER_SET].each do |exDriver|
                        if driver == exDriver
                            exclude = true
                            break
                        end
                    end
                end

                if not exclude
                    driverArray << driver
                end
            end
        end

        return driverArray
    end

    def LCKTestDriver.getRandomLongLivedDriver(params = {})
        driverArray = LCKTestDriver.getLongLivedDrivers(params)
        return driverArray[rand(driverArray.length())]
    end

    def LCKTestDriver.startNewShortLivedDriver(params = {})
        node = nil
        node = params[:NODE] if params[:NODE]
        driver = SAFTestDriver.startNewShortLivedDriver(params)
        return LCKTestDriver.new(:NODE => node, 
                                 :INSTANCE_ID => driver.getInstanceID())
    end

    def getRandomWorkerThread(params = {})
        if multiThreaded?()
            threads = getThreads(:SPEC => "LCK")
            workerThreads = []
            threads.each do |thread|
                if thread.workerThread?()
                    exclude = false
                    if params[:EXCLUDE_THREAD_SET]
                        params[:EXCLUDE_THREAD_SET].each do |exThread|
                            if exThread.getThreadID() == thread.getThreadID()
                                exclude = true
                                break
                            end
                        end
                    end

                    if not exclude
                        workerThreads << thread
                    end
                end
            end
        else
            raise "For now you can only call this on multi-threaded drivers"
        end

        return workerThreads[rand(workerThreads.length())]
    end

    def getAllSessions(params = {})
        sessionArray = []
        results = runDriver(params.update({:OP => "STATUS_REQ",
                                           :KVP_HASH => {}}))
        numSessions = results["NUM_LCK_SESSIONS"].to_i
        0.upto(numSessions - 1) do |n|
            sessionID = results["LCK_SESSION_#{n}_ID"]
            session = LCKSession.new(sessionID)
            dispatchFlags = results["LCK_SESSION_#{n}_DISPATCH_FLAGS"]
            session.setDispatchFlags(dispatchFlags)
            isLongLived = results["LCK_SESSION_#{n}_LONG_LIVED"]
            if isLongLived == "TRUE"
                session.setLongLived(true)
            else
                session.setLongLived(false)
            end

            isGlobal = results["LCK_SESSION_#{n}_GLOBAL"]
            if isGlobal == "TRUE"
                session.setGlobal(true)
            else
                session.setGlobal(false)
            end

            # Test cases get thread-local sessions unless they specifically
            # ask for a global session
            if (not session.global?()) or
                (params[:GLOBAL] and session.global?())
                sessionArray << session
            end
        end
        return sessionArray
    end

    def getAllLongLivedSessions(params = {})
        sessionArray = getAllSessions(params)
        longLivedArray = []
        sessionArray.each do |s|
            if s.longLived?()
                if params[:DISPATCH_FLAGS]
                    if params[:DISPATCH_FLAGS] == s.getDispatchFlags()
                        longLivedArray << s
                    end
                else
                    # If they don't ask for a specific DISPATCH_FLAGS, we'll
                    # give the one that is either DISPATCH_ALL or DISPATCH_ONE
                    # so that they don't end up with something that doesn't
                    # auto dispatch.
                    if "SA_DISPATCH_ONE" == s.getDispatchFlags() or \
                       "SA_DISPATCH_ALL" == s.getDispatchFlags()
                        longLivedArray << s
                    end
                end
            end
        end
        return longLivedArray
    end

    def getRandomLongLivedSession(params = {})
        longLivedArray = getAllLongLivedSessions(params)
        return longLivedArray[rand(longLivedArray.length())]
    end

    def init(params = {})
        kvpHash = {'LCK_SESSION_ID' => params[:SESSION].getID(),
                   'DISPATCH_FLAGS' => params[:DISPATCH_FLAGS],
                   'LOCK_WAITER_CB' => 'TRUE',
                   'LOCK_GRANT_CB' => 'TRUE',
                   'RESOURCE_OPEN_CB' => 'TRUE',
                   'RESOURCE_UNLOCK_CB' => 'TRUE',
                   'VERSION_RELEASE_CODE' => SAFTestUtils.SA_AIS_RELEASE_CODE,
                   'VERSION_MAJOR' => SAFTestUtils.SA_AIS_MAJOR_VERSION,
                   'VERSION_MINOR' => SAFTestUtils.SA_AIS_MINOR_VERSION,
                   'NULL_LCK_HANDLE' => 'FALSE',
                   'NULL_CALLBACKS' => 'FALSE',
                   'NULL_VERSION' => 'FALSE'}
        if params[:NULL_LCK_HANDLE]
            kvpHash['NULL_LCK_HANDLE'] = 'TRUE'
        end

        if params[:NULL_CALLBACKS]
            kvpHash['NULL_CALLBACKS'] = 'TRUE'
        else
            if params[:NO_LOCK_WAITER_CB]
                kvpHash['LOCK_WAITER_CB'] = 'FALSE'
            end
            if params[:NO_LOCK_GRANT_CB]
                kvpHash['LOCK_GRANT_CB'] = 'FALSE'
            end
            if params[:NO_RESOURCE_OPEN_CB]
                kvpHash['RESOURCE_OPEN_CB'] = 'FALSE'
            end
            if params[:NO_RESOURCE_UNLOCK_CB]
                kvpHash['RESOURCE_UNLOCK_CB'] = 'FALSE'
            end
        end

        if params[:NULL_VERSION]
            kvpHash['NULL_VERSION'] = 'TRUE'
        end

        if params[:VERSION_RELEASE_CODE]
            kvpHash['VERSION_RELEASE_CODE'] = params[:VERSION_RELEASE_CODE]
        end

        if params[:VERSION_MAJOR]
            kvpHash['VERSION_MAJOR'] = params[:VERSION_MAJOR]
        end

        if params[:VERSION_MINOR]
            kvpHash['VERSION_MINOR'] = params[:VERSION_MINOR]
        end

        runDriver(params.update({:OP => "INITIALIZE_REQ", 
                                 :KVP_HASH => kvpHash}))
    end

    def finalize(params = {})
        kvpHash = {'LCK_SESSION_ID' => params[:SESSION].getID()}
        runDriver(params.update({:OP => "FINALIZE_REQ", :KVP_HASH => kvpHash}))
    end

    def selectObjectGet(params = {})
        kvpHash = {'LCK_SESSION_ID' => params[:SESSION].getID(),
                   'NULL_SELECTION_OBJECT' => 'FALSE'}
        if params[:NULL_SELECTION_OBJECT]
            kvpHash['NULL_SELECTION_OBJECT'] = 'TRUE'
        end
        runDriver(params.update({:OP => "SELECTION_OBJECT_GET_REQ", 
                                 :KVP_HASH => kvpHash}))

    end

    def dispatch(params = {})
        kvpHash = {'LCK_SESSION_ID' => params[:SESSION].getID(),
                   'DISPATCH_FLAGS' => params[:DISPATCH_FLAGS]}
        runDriver(params.update({:OP => "DISPATCH_REQ", :KVP_HASH => kvpHash}))
    end

    def resourceOpen(params = {})
        kvpHash = {'LCK_SESSION_ID' => params[:SESSION].getID(),
                   'LOCK_NAME' => params[:LOCK_NAME]}
        runDriver(params.update({:OP => "LOCK_RESOURCE_OPEN_REQ", 
                                 :KVP_HASH => kvpHash}))
    end

    def resourceOpenAsync(params = {})
        kvpHash = {'LCK_SESSION_ID' => params[:SESSION].getID(),
                   'INVOCATION' => params[:INVOCATION],
                   'LOCK_NAME' => params[:LOCK_NAME]}

        runDriver(params.update({:OP => "LOCK_RESOURCE_OPEN_ASYNC_REQ", 
                                 :KVP_HASH => kvpHash}))
    end

    def resourceClose(params = {})
        kvpHash = {'LCK_SESSION_ID' => params[:SESSION].getID()}
        runDriver(params.update({:OP => "LOCK_RESOURCE_CLOSE_REQ", 
                                 :KVP_HASH => kvpHash}))
    end

private

    def getLockKVPHash(params = {})
        waiterSignal = 0
        if params[:WAITER_SIGNAL]
            waiterSignal = params[:WAITER_SIGNAL]
        end
        timeout = "SA_TIME_MAX"
        if params[:TIMEOUT]
            timeout = params[:TIMEOUT]
        end
        kvpHash = {'LCK_SESSION_ID' => params[:SESSION].getID(),
                   'LOCK_MODE' => params[:LOCK_MODE],
                   'WAITER_SIGNAL' => waiterSignal,
                   'TIMEOUT' => timeout,
                   'NULL_LOCK_ID' => 'FALSE',
                   'NULL_LOCK_STATUS' => 'FALSE',
                   'LOCK_FLAG_NO_QUEUE' => 'FALSE',
                   'LOCK_FLAG_ORPHAN' => 'FALSE',
                   'LOCK_FLAG_INVALID' => 'FALSE'}
        if params[:EXPECTED_LOCK_STATUS]
            kvpHash['EXPECTED_LOCK_STATUS'] = params[:EXPECTED_LOCK_STATUS]
        end
        if params[:INVOCATION]
            # Async requests only
            kvpHash['INVOCATION'] = params[:INVOCATION]
        end
        if params[:LOCK_FLAG_NO_QUEUE]
            kvpHash['LOCK_FLAG_NO_QUEUE'] = 'TRUE'
        end
        if params[:LOCK_FLAG_ORPHAN]
            kvpHash['LOCK_FLAG_ORPHAN'] = 'TRUE'
        end
        if params[:LOCK_FLAG_INVALID]
            kvpHash['LOCK_FLAG_INVALID'] = 'TRUE'
        end
        if params[:NULL_LOCK_ID]
            kvpHash['NULL_LOCK_ID'] = 'TRUE'
        end
        if params[:NULL_LOCK_STATUS]
            kvpHash['NULL_LOCK_STATUS'] = 'TRUE'
        end
        return kvpHash
    end

public

    def lockSync(params = {})
        kvpHash = getLockKVPHash(params)
        runDriver(params.update({:OP => "LOCK_SYNC_REQ", 
                                 :KVP_HASH => kvpHash}))
    end

    def lockSyncBG(params = {})
        kvpHash = getLockKVPHash(params)
        return runDriverBG(params.update({:OP => "LOCK_SYNC_REQ", 
                                          :KVP_HASH => kvpHash}))
    end

    def lockAsync(params = {})
        kvpHash = getLockKVPHash(params)
        runDriver(params.update({:OP => "LOCK_ASYNC_REQ", 
                                 :KVP_HASH => kvpHash}))
    end

    def unlockSync(params = {})
        kvpHash = {'LCK_SESSION_ID' => params[:SESSION].getID()}
        runDriver(params.update({:OP => "UNLOCK_SYNC_REQ", 
                                 :KVP_HASH => kvpHash}))
    end

    def unlockAsync(params = {})
        kvpHash = {'LCK_SESSION_ID' => params[:SESSION].getID(),
                   'INVOCATION' => params[:INVOCATION]}
        runDriver(params.update({:OP => "UNLOCK_ASYNC_REQ", 
                                 :KVP_HASH => kvpHash}))
    end

    def lockGetWaitCount(params = {})
        kvpHash = {'LCK_SESSION_ID' => params[:SESSION].getID()}
        kvpResults = runDriver(params.update({:OP => "LOCK_GET_WAIT_COUNT_REQ", 
                                              :KVP_HASH => kvpHash}))
        lastWaiterSignal = kvpResults['LAST_DELIVERED_WAITER_SIGNAL'].to_i
        waiterNotifyCount = kvpResults['WAITER_SIGNAL_NOTIFICATION_COUNT'].to_i
        if lastWaiterSignal != params[:EXPECTED_LAST_WAITER_SIGNAL]
            raise "Expected waiter signal %d, got %d." % \
                  [params[:EXPECTED_LAST_WAITER_SIGNAL], lastWaiterSignal]
        end
        if waiterNotifyCount != params[:EXPECTED_NOTIFICATION_COUNT]
            raise "Expected notify count %d, got %d." % \
                  [params[:EXPECTED_NOTIFICATION_COUNT], waiterNotifyCount]
        end
    end
end # class

end # module
