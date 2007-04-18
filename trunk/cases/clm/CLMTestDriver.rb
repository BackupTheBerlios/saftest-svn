module SAFTest

require 'SAFTestDriver'
require 'SAFTestUtils'

class CLMSession < SAFTestUtils
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

class CLMTestDriver < SAFTestDriver
    def initialize(params = {})
        super(params)
    end

private

    def createSession(params = {})
        kvpHash = {"CLM_SESSION_LONG_LIVED" => "FALSE",
                   "CLM_SESSION_GLOBAL" => "FALSE"}
        if params[:LONG_LIVED]
            kvpHash["CLM_SESSION_LONG_LIVED"] = "TRUE"
        end
        if params[:GLOBAL]
            kvpHash["CLM_SESSION_GLOBAL"] = "TRUE"
        end
        results = runDriver(params.update({:OP => "CREATE_SESSION_REQ", 
                                           :KVP_HASH => kvpHash}))
        sessionID = results["CLM_SESSION_ID"].to_i
        session = CLMSession.new(sessionID)
        return session
    end

public

    # This needs to be public in order to be called from the base class
    def initializeResources()
        threads = getThreads(:SPEC => "CLM")
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
        return 'clm_driver'
    end

    def waitForClusterNodeGetCBCount(params = {})
        expectedCount = params[:EXPECTED_COUNT]
        waitFor(@@WAIT_INTERVAL, @@WAIT_TIMEOUT, 
                "cluster node get callback count to be #{expectedCount}") {
            count = clusterNodeGetCBCount(:SESSION => params[:SESSION])
            count == expectedCount
        }
    end

    def waitForClusterTrackCBCount(params = {})
        expectedCount = params[:EXPECTED_COUNT]
        waitFor(@@WAIT_INTERVAL, @@WAIT_TIMEOUT, 
                "cluster track callback count to be #{expectedCount}") {
            count = clusterTrackCBCount(:SESSION => params[:SESSION])
            count == expectedCount
        }
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
        kvpHash = {'CLM_SESSION_ID' => params[:SESSION].getID()}
        results = runDriver(params.update({:OP => "DELETE_SESSION_REQ", 
                                           :KVP_HASH => kvpHash}))
    end

    def CLMTestDriver.getLongLivedDrivers(params = {})
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
            driver = CLMTestDriver.new(:NODE => node, :INSTANCE_ID => n)

            # Test cases get single-threaded drivers unless they specifically
            # ask for a multi-threaded driver
            if (not params[:THREADED] and not driver.multiThreaded?()) or 
                (params[:THREADED] and driver.multiThreaded?())
                exclude = false
                if params[:EXCLUDE_DRIVER_SET]
                    params[:EXCLUDE_DRIVER_SET].each do |exDriver|
                        if driver == exDriver.getThreadID()
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

    def CLMTestDriver.getRandomLongLivedDriver(params = {})
        driverArray = CLMTestDriver.getLongLivedDrivers(params)
        return driverArray[rand(driverArray.length())]
    end

    def getRandomWorkerThread(params = {})
        if multiThreaded?()
            threads = getThreads(:SPEC => "CLM")
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
        numSessions = results["NUM_CLM_SESSIONS"].to_i
        0.upto(numSessions - 1) do |n|
            sessionID = results["CLM_SESSION_#{n}_ID"]
            session = CLMSession.new(sessionID)
            dispatchFlags = results["CLM_SESSION_#{n}_DISPATCH_FLAGS"]
            session.setDispatchFlags(dispatchFlags)
            isLongLived = results["CLM_SESSION_#{n}_LONG_LIVED"]
            if isLongLived == "TRUE"
                session.setLongLived(true)
            else
                session.setLongLived(false)
            end

            isGlobal = results["CLM_SESSION_#{n}_GLOBAL"]
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
        sessionArray = getAllSessions()
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

    # The Main test session is the first long lived one.  This is the
    # session that can call saClmClusterTrack with the TRACK_CURRENT flag
    # and expect the results via a callback.
    def getMainLongLivedSession()
        longLivedArray = getAllLongLivedSessions()
        return longLivedArray[0]
    end

    def compareCluster(cluster, validateCluster)
        if cluster != validateCluster
            log("cluster (from file)")
            cluster.display()
            log("cluster (from command)")
            validateCluster.display()
            failed("cluster != validateCluster")
        end
    end

    def compareNodes(node, validateNode)
        if node != validateNode
            log("node (from file)")
            node.display()
            log("node (from command)")
            validateNode.display()
            failed("node != validateNode")
        end
    end

    def compareDeletedNode(node, validateNode, nodeChangeFlag)
        if validateNode != nil
            validateNode.display()
            failed("validateNode != nil")
        end

        if node == nil
            failed("expected to find node in callback")
        end

        if (nodeChangeFlag != 'SA_CLM_NODE_RECONFIGURED')
            failed("nodeChangeFlag != SA_CLM_NODE_RECONFIGURED")
        end
    end

    def populateChangeFlag(newClusterFromCmd, 
                           oldClusterFromCmd, 
                           callBackCluster)

       callBackCluster.getNodes().each do |node|
             nodeId = node.getID()

             oldClusterNode = oldClusterFromCmd.getNodeById(nodeId)
             newClusterNode = newClusterFromCmd.getNodeById(nodeId)

             if (newClusterNode == nil)
                # node is deleted from the cluster
                next
             end 

             newClusterNodeChangeFlag = 
                   newClusterFromCmd.getNodeChangeFlagByNodeId(nodeId)

             if (oldClusterNode == nil)
                # new node added to the cluster
                newClusterNodeChangeFlag.setChangeFlag('SA_CLM_NODE_RECONFIGURED')
                next
             end


             #
             # check membership status
             #
             oldNodeMemberFlag = oldClusterNode.getMember()
             newNodeMemberFlag = newClusterNode.getMember()
             if (oldNodeMemberFlag == newNodeMemberFlag)
                newClusterNodeChangeFlag.setChangeFlag('SA_CLM_NODE_NO_CHANGE')
                next
             end
             if (oldNodeMemberFlag)
                newClusterNodeChangeFlag.setChangeFlag('SA_CLM_NODE_LEFT')
                next
             end
             if (newNodeMemberFlag)
                newClusterNodeChangeFlag.setChangeFlag('SA_CLM_NODE_JOINED')
                next
             end
       end
    end

    def init(params = {})
        kvpHash = {'CLM_SESSION_ID' => params[:SESSION].getID(),
                   'DISPATCH_FLAGS' => params[:DISPATCH_FLAGS],
                   'CLUSTER_NODE_GET_CB' => 'TRUE',
                   'CLUSTER_TRACK_CB' => 'TRUE',
                   'VERSION_RELEASE_CODE' => SAFTestUtils.SA_AIS_RELEASE_CODE,
                   'VERSION_MAJOR' => SAFTestUtils.SA_AIS_MAJOR_VERSION,
                   'VERSION_MINOR' => SAFTestUtils.SA_AIS_MINOR_VERSION,
                   'DISPATCH_FLAGS' => params[:DISPATCH_FLAGS],
                   'NULL_CLM_HANDLE' => 'FALSE',
                   'NULL_CALLBACKS' => 'FALSE',
                   'NULL_VERSION' => 'FALSE'}

        if params[:NULL_CLM_HANDLE]
            kvpHash['NULL_CLM_HANDLE'] = 'TRUE'
        end

        if params[:NULL_CALLBACKS]
            kvpHash['NULL_CALLBACKS'] = 'TRUE'
        else
            if params[:NO_CLUSTER_NODE_GET_CB]
                kvpHash['CLUSTER_NODE_GET_CB'] = 'FALSE'
            end

            if params[:NO_CLUSTER_TRACK_CB]
                kvpHash['CLUSTER_TRACK_CB'] = 'FALSE'
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
        kvpHash = {'CLM_SESSION_ID' => params[:SESSION].getID()}
        runDriver(params.update({:OP => "FINALIZE_REQ", 
                                 :KVP_HASH => kvpHash}))
    end

    def selectObjectGet(params = {})
        kvpHash = {'CLM_SESSION_ID' => params[:SESSION].getID(),
                   'NULL_SELECTION_OBJECT' => 'FALSE'}
        if params[:NULL_SELECTION_OBJECT]
            kvpHash['NULL_SELECTION_OBJECT'] = 'TRUE'
        end
        runDriver(params.update({:OP => "SELECTION_OBJECT_GET_REQ", 
                                 :KVP_HASH => kvpHash}))
    end

    def dispatch(params = {})
        kvpHash = {'CLM_SESSION_ID' => params[:SESSION].getID(),
                   'DISPATCH_FLAGS' => params[:DISPATCH_FLAGS]}
        runDriver(params.update({:OP => "DISPATCH_REQ", 
                                 :KVP_HASH => kvpHash}))
    end

    def clusterNodeGet(params = {})
        xmlFile = nil

        timeout = "SA_TIME_MAX"
        if params[:TIMEOUT]
            timeout = params[:TIMEOUT]
        end
        kvpHash = {'CLM_SESSION_ID' => params[:SESSION].getID(),
                   'NODE_ID' => params[:NODE_ID],
                   'NULL_CLUSTER_NODE' => 'FALSE',
                   'TIMEOUT' => timeout}
        if params[:NULL_CLUSTER_NODE]
            kvpHash['NULL_CLUSTER_NODE'] = 'TRUE'
        end

        if params[:VALIDATE_NODE]
            xmlFile = tmpFile("cluster_node_local")
            kvpHash['XML_FILE'] = xmlFile
        end

        runDriver(params.update({:OP => "CLUSTER_NODE_GET_REQ", 
                                 :KVP_HASH => kvpHash}))
        if params[:VALIDATE_NODE]
            node = getImplementation().getNodeFromFile(xmlFile)
            compareNodes(node, params[:VALIDATE_NODE])
            File.unlink(xmlFile)
        end
    end

    def generateInvocation()
        invocation = rand(1000)
        if invocation == 0
            invocation += 1
        end
        return invocation
    end

    def clusterNodeGetAsyncWithInvocation(params = {})
        xmlFile = nil
        kvpHash = {'CLM_SESSION_ID' => params[:SESSION].getID(),
                   'INVOCATION' => params[:INVOCATION],
                   'NODE_ID' => params[:NODE_ID]}
        if params[:VALIDATE_NODE]
            xmlFile = tmpFile("cluster_node_async_local")
            kvpHash['XML_FILE'] = xmlFile
        end

        runDriver(params.update({:OP => "CLUSTER_NODE_GET_ASYNC_REQ", 
                                 :KVP_HASH => kvpHash}))
        if params[:VALIDATE_NODE]
            print "Going to wait for the XML file\n"
            waitFor(@@WAIT_INTERVAL, @@WAIT_TIMEOUT, "#{xmlFile} to exist") {
                # !!! This should be made to validate against the schema
                FileTest.exist?(xmlFile) and not File.stat(xmlFile).zero?
            }
            node = getImplementation().getNodeFromFile(xmlFile)
            compareNodes(node, params[:VALIDATE_NODE])
            File.unlink(xmlFile)
        end
    end

    def clusterNodeGetAsync(params = {})
        invocation = generateInvocation()
        params[:INVOCATION] = invocation
        clusterNodeGetAsyncWithInvocation(params)
    end

    def clusterNodeGetResetCBCount(params = {})
        kvpHash = {'CLM_SESSION_ID' => params[:SESSION].getID()}
        kvpResults = runDriver(params.update(
            {:OP => 'CLUSTER_NODE_GET_RESET_CALLBACK_COUNT_REQ',
             :KVP_HASH => kvpHash}))
    end

    def clusterNodeGetCBCount(params = {})
        kvpHash = {'CLM_SESSION_ID' => params[:SESSION].getID()}
        kvpResults = runDriver(params.update(
            {:OP => 'CLUSTER_NODE_GET_CALLBACK_COUNT_REQ', 
             :KVP_HASH => kvpHash}))
        cbCount = kvpResults['CLUSTER_NODE_GET_CALLBACK_COUNT']
        if params[:EXPECTED_COUNT]
            if cbCount.to_i != params[:EXPECTED_COUNT]
               raise "Expected count %d, got %d" % [params[:EXPECTED_COUNT], 
                                                    cbCount.to_i]
            end 
        else
            return cbCount.to_i
        end
    end

    def clusterTrackResetCBCount(params = {})
        kvpHash = {'CLM_SESSION_ID' => params[:SESSION].getID()}
        kvpResults = runDriver(params.update(
            {:OP => 'CLUSTER_TRACK_RESET_CALLBACK_COUNT_REQ',
             :KVP_HASH => kvpHash}))
    end

    def clusterTrackCBCount(params = {})
        kvpHash = {'CLM_SESSION_ID' => params[:SESSION].getID()}
        kvpResults = runDriver(params.update(
            {:OP => 'CLUSTER_TRACK_CALLBACK_COUNT_REQ', 
             :KVP_HASH => kvpHash}))
        cbCount = kvpResults['CLUSTER_TRACK_CALLBACK_COUNT']
        if params[:EXPECTED_COUNT]
            if cbCount.to_i != params[:EXPECTED_COUNT]
               raise "Expected count %d, got %d" % [params[:EXPECTED_COUNT], 
                                                    cbCount.to_i]
            end 
        else
            return cbCount.to_i
        end
    end

    def clusterNodeGetAsyncInvocation(params = {})
        kvpHash = {'CLM_SESSION_ID' => params[:SESSION].getID()}
        kvpResults = runDriver(params.update(
            {:OP => 'CLUSTER_NODE_GET_ASYNC_INVOCATION_REQ',
             :KVP_HASH => kvpHash}))
        ret = array[0]
        lines = array[1]
        invocation = kvpResults['CLUSTER_NODE_GET_ASYNC_INVOCATION'].to_i
        if invocation != params[:EXPECTED_INVOCATION]
            raise "Expected invocation %d, got %d." % \
                  [params[:EXPECTED_INVOCATION], invocation]
        end
    end

    def clusterTrack(params = {})
        numberOfItems = 0
        if params[:NUMBER_OF_ITEMS]
            numberOfItems = params[:NUMBER_OF_ITEMS]
        end
        kvpHash = {'CLM_SESSION_ID' => params[:SESSION].getID(),
                   'TRACK_CURRENT' => 'FALSE',
                   'TRACK_CHANGES' => 'FALSE',
                   'TRACK_CHANGES_ONLY' => 'FALSE',
                   'INVALID_TRACK_FLAGS' => 'FALSE',
                   'NULL_NOTIFICATION_BUFFER' => 'FALSE',
                   'NULL_CLUSTER_NOTIFICATION' => 'FALSE',
                   'NUMBER_OF_ITEMS' => numberOfItems}
        if params[:TRACK_CURRENT]
            kvpHash['TRACK_CURRENT'] = 'TRUE'
        end
        if params[:TRACK_CHANGES]
            kvpHash['TRACK_CHANGES'] = 'TRUE'
        end
        if params[:TRACK_CHANGES_ONLY]
            kvpHash['TRACK_CHANGES_ONLY'] = 'TRUE'
        end
        if params[:INVALID_TRACK_FLAGS]
            kvpHash['INVALID_TRACK_FLAGS'] = 'TRUE'
        end
        if params[:NULL_NOTIFICATION_BUFFER]
            kvpHash['NULL_NOTIFICATION_BUFFER'] = 'TRUE'
        end
        if params[:NULL_CLUSTER_NOTIFICATION]
            kvpHash['NULL_CLUSTER_NOTIFICATION'] = 'TRUE'
        end

        xmlFile = tmpFile("cluster_track")
        kvpHash['XML_FILE'] = xmlFile

        runDriver(params.update({:OP => "CLUSTER_TRACK_REQ", 
                                 :KVP_HASH => kvpHash}))

        # Setting both of these is not allowed by the spec, so this is for
        # testing the error condition
        if params[:TRACK_CHANGES_ONLY] and params[:TRACK_CHANGES]
           return
        end

        if params[:TRACK_CURRENT] and params[:VALIDATE_CLUSTER] != nil
            cluster = getImplementation().getClusterFromFile(xmlFile)
            compareCluster(cluster, params[:VALIDATE_CLUSTER])
            File.unlink(xmlFile)
        end

        if params[:TRACK_CHANGES_ONLY] or params[:TRACK_CHANGES]
            # Cause a reformation
            delNodeFlag = 0
            oldClusterFromCmd = getImplementation().getCluster()
            @configuredNodeNames = 
                   @config.getArrValue('main', 'testNodes').split(' ')
            if oldClusterFromCmd.getNodes().length <  
                   @configuredNodeNames.length
               nodeName = oldClusterFromCmd.getRandomUnconfiguredNodeName()
               if nodeName != nil
                  # select add node scenario
                  getImplementation().addNode(nodeName)
               end
            else
                downNode = oldClusterFromCmd.getRandomNode(
                    :STATUS => "down",
                    :SHORT_LIVED => true,
                    :EXCLUDE_NODE_SET => [oldClusterFromCmd.getLocalNode()])
                if downNode != nil
                    nodeName = downNode.getName()
                    #
                    # Randomly select between startNode or deleteNode scenario
                    #
                    num = rand(2)
                    if num == 0
                       getImplementation().startNode(nodeName)
                    else
                       delNodeFlag = 1
                       getImplementation().deleteNode(nodeName)
                    end
                else
                    upNode = oldClusterFromCmd.getRandomNode(
                        :STATUS => "up",
                        :SHORT_LIVED => true,
                        :EXCLUDE_NODE_SET => [oldClusterFromCmd.getLocalNode()])
                    nodeName = upNode.getName()
 
                    # select halt node scenario
                    getImplementation().stopNode(nodeName)
                end
            end

            print "Going to wait for the XML file\n"
            waitFor(@@WAIT_INTERVAL, @@WAIT_TIMEOUT, "#{xmlFile} to exist") {
                FileTest.exist?(xmlFile) and not File.stat(xmlFile).zero?
            }
        end

        if params[:TRACK_CHANGES]
            newClusterFromCmd = getImplementation().getCluster()
            cluster = getImplementation().getClusterFromFile(xmlFile)
            if (delNodeFlag == 1)
                cluster.getNodes().each do |node|
                   validateNode = 
                        newClusterFromCmd.getNodeByName(node.getName())
                   if (node.getName() == nodeName) 
                      compareDeletedNode(node, validateNode, 
                                         'SA_CLM_NODE_RECONFIGURED')
                   else
                      populateChangeFlag(newClusterFromCmd, 
                                         oldClusterFromCmd, cluster);
                      compareNodes(node, validateNode)
                   end
                end
            else 
                populateChangeFlag(newClusterFromCmd, 
                                   oldClusterFromCmd, cluster);
                compareCluster(cluster, newClusterFromCmd)
            end
            File.unlink(xmlFile)
        end

        if params[:TRACK_CHANGES_ONLY]
            newClusterFromCmd = getImplementation().getCluster()
            cluster = getImplementation().getClusterFromFile(xmlFile)
            node = cluster.getNodeByName(nodeName)
            validateNode = newClusterFromCmd.getNodeByName(nodeName)
            if (delNodeFlag == 1) 
                compareDeletedNode(node, validateNode, 
                                   'SA_CLM_NODE_RECONFIGURED')
            else 
                populateChangeFlag(newClusterFromCmd, 
                                   oldClusterFromCmd, cluster);
                compareNodes(node, validateNode)
            end
            File.unlink(xmlFile)
        end
    end

    def clusterTrackWithNoReformation(params = {})
        kvpHash = {'CLM_SESSION_ID' => params[:SESSION].getID(),
                   'TRACK_CURRENT' => 'FALSE',
                   'TRACK_CHANGES' => 'FALSE',
                   'TRACK_CHANGES_ONLY' => 'FALSE',
                   'INVALID_TRACK_FLAGS' => 'FALSE',
                   'NULL_NOTIFICATION_BUFFER' => 'FALSE',
                   'NULL_CLUSTER_NOTIFICATION' => 'FALSE',
                   'NUMBER_OF_ITEMS' => params[:NUMBER_OF_ITEMS]}
        if params[:TRACK_CURRENT]
            kvpHash['TRACK_CURRENT'] = 'TRUE'
        end
        if params[:TRACK_CHANGES]
            kvpHash['TRACK_CHANGES'] = 'TRUE'
        end
        if params[:TRACK_CHANGES_ONLY]
            kvpHash['TRACK_CHANGES_ONLY'] = 'TRUE'
        end
        if params[:INVALID_TRACK_FLAGS]
            kvpHash['INVALID_TRACK_FLAGS'] = 'TRUE'
        end
        if params[:NULL_NOTIFICATION_BUFFER]
            kvpHash['NULL_NOTIFICATION_BUFFER'] = 'TRUE'
        end
        if params[:NULL_CLUSTER_NOTIFICATION]
            kvpHash['NULL_CLUSTER_NOTIFICATION'] = 'TRUE'
        end

        xmlFile = tmpFile("cluster_track")
        kvpHash['XML_FILE'] = xmlFile

        runDriver(params.update({:OP => "CLUSTER_TRACK_REQ", 
                                 :KVP_HASH => kvpHash}))

        if params[:TRACK_CHANGES_ONLY] and params[:TRACK_CHANGES]
           return
        end

        if params[:TRACK_CURRENT] and params[:VALIDATE_CLUSTER] != nil
            cluster = getImplementation().getClusterFromFile(xmlFile)
            compareCluster(cluster, params[:VALIDATE_CLUSTER])
            File.unlink(xmlFile)
        end
    end
    
    def clusterTrackStop(params = {})
        kvpHash = {'CLM_SESSION_ID' => params[:SESSION].getID()}
        array = runDriver(params.update({:OP => 'CLUSTER_TRACK_STOP_REQ', 
                                         :KVP_HASH => kvpHash}))
    end

    def displayLastNotificationBuffer(params = {})
        kvpHash = {'CLM_SESSION_ID' => params[:SESSION].getID(),
                    'XML_FILE' => xmlFile}
        array = runDriver(params.update(
            {:OP => 'DISPLAY_LAST_NOTIFICATION_BUFFER_REQ', 
             :KVP_HASH => kvpHash}))
    end
end # class

end # module
