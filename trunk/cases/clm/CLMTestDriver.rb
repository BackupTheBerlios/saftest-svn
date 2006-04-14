module SAFTest

require 'SAFTestDriver'
require 'SAFTestUtils'

class CLMTestResource < SAFTestUtils
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

class CLMTestDriver < SAFTestDriver
    @@nextInstanceID = 1

    def initialize(node=nil, instanceID=0)
        super(node, nil, instanceID)
    end

private

    def createTestResource(longLived)
        kvpHash = {"CLM_RESOURCE_LONG_LIVED" => "FALSE"}
        if longLived == true
            kvpHash["CLM_RESOURCE_LONG_LIVED"] = "TRUE"
        end
        results = runDriver("CREATE_TEST_RESOURCE_REQ", kvpHash, 
                                SAFTestUtils.SA_AIS_OK)
        resourceID = results["CLM_RESOURCE_ID"].to_i
        resource = CLMTestResource.new(resourceID)
        return resource
    end

public

    def getName()
        return 'clm_driver'
    end

    def waitForClusterNodeGetCBCount(resource, expectedCount)
        waitFor(@@WAIT_INTERVAL, @@WAIT_TIMEOUT, 
                "cluster node get callback count to be #{expectedCount}") {
            count = clusterNodeGetCBCount(resource, nil)
            count == expectedCount
        }
    end

    def waitForClusterTrackCBCount(resource, expectedCount)
        waitFor(@@WAIT_INTERVAL, @@WAIT_TIMEOUT, 
                "cluster track callback count to be #{expectedCount}") {
            count = clusterTrackCBCount(resource, nil)
            count == expectedCount
        }
    end

    def createLongLivedTestResource()
        createTestResource(true)
    end

    def createShortLivedTestResource()
        createTestResource(false)
    end

    def deleteTestResource(resource)
        kvpHash = {'CLM_RESOURCE_ID' => resource.getID()}
        results = runDriver("DELETE_TEST_RESOURCE_REQ", kvpHash, 
                            SAFTestUtils.SA_AIS_OK)
    end

    def CLMTestDriver.getLongLivedDrivers(node)
        utils = SAFTestUtils.new
        config = SAFTestConfig.new
        config.loadFromXML(utils.configXMLFile())

        numDrivers = config.getIntValue('main', 'numLongLivedDrivers')
        driverArray = []
        1.upto(numDrivers) do |n|
            driver = CLMTestDriver.new(node, n)
            driverArray << driver
        end
        return driverArray
    end

    def CLMTestDriver.getRandomLongLivedDriver(node)
        driverArray = CLMTestDriver.getLongLivedDrivers(node)
        return driverArray[0]
    end

    def getAllTestResources()
        resourceArray = []
        results = runDriver("STATUS_REQ", {},
                            SAFTestUtils.SA_AIS_OK)
        numResources = results["NUM_CLM_RESOURCES"].to_i
        0.upto(numResources - 1) do |n|
            resourceID = results["CLM_RESOURCE_#{n}_ID"]
            resource = CLMTestResource.new(resourceID)
            dispatchFlags = results["CLM_RESOURCE_#{n}_DISPATCH_FLAGS"]
            resource.setDispatchFlags(dispatchFlags)
            isLongLived = results["CLM_RESOURCE_#{n}_LONG_LIVED"]
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

    def init(resource, setClusterNodeGetCB, setClusterTrackCB,
             dispatchFlags, expectedReturn)
        kvpHash = {'CLM_RESOURCE_ID' => resource.getID(),
                    'DISPATCH_FLAGS' => dispatchFlags,
                    'CLUSTER_NODE_GET_CB' => 'FALSE',
                    'CLUSTER_TRACK_CB' => 'FALSE',
                    'VERSION_RELEASE_CODE' => SAFTestUtils.SA_AIS_RELEASE_CODE,
                    'VERSION_MAJOR' => SAFTestUtils.SA_AIS_MAJOR_VERSION,
                    'VERSION_MINOR' => SAFTestUtils.SA_AIS_MINOR_VERSION,
                    'DISPATCH_FLAGS' => dispatchFlags,
                    'NULL_CLM_HANDLE' => 'FALSE',
                    'NULL_CALLBACKS' => 'FALSE',
                    'NULL_VERSION' => 'FALSE'}

        if setClusterNodeGetCB
            kvpHash['CLUSTER_NODE_GET_CB'] = 'TRUE'
        end
        if setClusterTrackCB
            kvpHash['CLUSTER_TRACK_CB'] = 'TRUE'
        end
        runDriver("INITIALIZE_REQ", kvpHash, expectedReturn)
    end

    def initWithOptions(resource, dispatchFlags, 
                        releaseCode, majorVersion, minorVersion,
                        nullClmHandle, nullCallbacks, nullVersion,
                        expectedReturn)
        kvpHash = {'CLM_RESOURCE_ID' => resource.getID(),
                   'VERSION_RELEASE_CODE' => releaseCode,
                   'VERSION_MAJOR' => majorVersion,
                   'VERSION_MINOR' => minorVersion,
                   'NULL_CLM_HANDLE' => 'FALSE',
                   'NULL_VERSION' => 'FALSE',
                   'DISPATCH_FLAGS' => dispatchFlags}

        if nullClmHandle
            kvpHash['NULL_CLM_HANDLE'] = 'TRUE'
        end

        if nullCallbacks
            kvpHash['NULL_CALLBACKS'] = 'TRUE'
        else
            kvpHash['NULL_CALLBACKS'] = 'FALSE'
            kvpHash['CLUSTER_NODE_GET_CB'] = 'TRUE'
            kvpHash['CLUSTER_TRACK_CB'] = 'TRUE'
        end

        if nullVersion
            kvpHash['NULL_VERSION'] = 'TRUE'
        end

        runDriver("INITIALIZE_REQ", kvpHash, expectedReturn)
    end

    def finalize(resource, expectedReturn)
        kvpHash = {'CLM_RESOURCE_ID' => resource.getID()}
        runDriver("FINALIZE_REQ", kvpHash, expectedReturn)
    end

    def selectObjectGet(resource, nullSelectionObject, expectedReturn)
        kvpHash = {'CLM_RESOURCE_ID' => resource.getID(),
                   'NULL_SELECTION_OBJECT' => 'FALSE'}
        if nullSelectionObject
            kvpHash['NULL_SELECTION_OBJECT'] = 'TRUE'
        end
        runDriver("SELECTION_OBJECT_GET_REQ", kvpHash, expectedReturn)
    end

    def dispatch(resource, dispatchFlags, expectedReturn)
        kvpHash = {'CLM_RESOURCE_ID' => resource.getID(),
                   'DISPATCH_FLAGS' => dispatchFlags}
        runDriver("DISPATCH_REQ", kvpHash, expectedReturn)
    end

    def clusterNodeGet(resource, nodeIDString, timeout, nullClusterNode,
                       validateNode, expectedReturn)
        xmlFile = nil
        kvpHash = {'CLM_RESOURCE_ID' => resource.getID(),
                    'NODE_ID' => nodeIDString,
                    'NULL_CLUSTER_NODE' => 'FALSE',
                    'TIMEOUT' => timeout}
        if nullClusterNode
            kvpHash['NULL_CLUSTER_NODE'] = 'TRUE'
        end

        if validateNode != nil
            xmlFile = tmpFile("cluster_node_local")
            kvpHash['XML_FILE'] = xmlFile
        end

        runDriver("CLUSTER_NODE_GET_REQ", kvpHash, expectedReturn)
        if validateNode != nil
            node = getImplementation().getNodeFromFile(xmlFile)
            compareNodes(node, validateNode)
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

    def clusterNodeGetAsyncWithInvocation(resource, invocation, 
                                          nodeIDString, validateNode,
                                          expectedReturn)
        xmlFile = nil
        kvpHash = {'CLM_RESOURCE_ID' => resource.getID(),
                    'INVOCATION' => invocation,
                    'NODE_ID' => nodeIDString}
        if validateNode != nil
            xmlFile = tmpFile("cluster_node_async_local")
            kvpHash['XML_FILE'] = xmlFile
        end

        runDriver("CLUSTER_NODE_GET_ASYNC_REQ", kvpHash, expectedReturn)
        if validateNode != nil
            print "Going to wait for the XML file\n"
            waitFor(@@WAIT_INTERVAL, @@WAIT_TIMEOUT, "#{xmlFile} to exist") {
                # !!! This should be made to validate against the schema
                FileTest.exist?(xmlFile) and not File.stat(xmlFile).zero?
            }
            node = getImplementation().getNodeFromFile(xmlFile)
            compareNodes(node, validateNode)
            File.unlink(xmlFile)
        end
    end

    def clusterNodeGetAsync(resource, nodeIDString, validateNode,
                            expectedReturn)
        invocation = generateInvocation()
        clusterNodeGetAsyncWithInvocation(resource, invocation, nodeIDString, 
                                          validateNode, expectedReturn)
    end

    def clusterNodeGetCBCount(resource, expectedCount)
        kvpHash = {'CLM_RESOURCE_ID' => resource.getID()}
        kvpResults = runDriver('CLUSTER_NODE_GET_CALLBACK_COUNT_REQ', kvpHash,
                                SAFTestUtils.SA_AIS_OK)
        cbCount = kvpResults['CLUSTER_NODE_GET_CALLBACK_COUNT']
        if expectedCount != nil
            if cbCount.to_i != expectedCount
               raise "Expected count %d, got %d" %  [expectedCount, cbCount.to_i]
            end 
        else
            return cbCount.to_i
        end
    end

    def clusterTrackResetCBCount(resource)
        kvpHash = {'CLM_RESOURCE_ID' => resource.getID()}
        kvpResults = runDriver('CLUSTER_TRACK_RESET_CALLBACK_COUNT_REQ', 
                                kvpHash,
                                SAFTestUtils.SA_AIS_OK)
    end

    def clusterTrackCBCount(resource, expectedCount)
        kvpHash = {'CLM_RESOURCE_ID' => resource.getID()}
        kvpResults = runDriver('CLUSTER_TRACK_CALLBACK_COUNT_REQ', kvpHash,
                                SAFTestUtils.SA_AIS_OK)
        cbCount = kvpResults['CLUSTER_TRACK_CALLBACK_COUNT']
        if expectedCount != nil
            if cbCount.to_i != expectedCount
               raise "Expected count %d, got %d" %  [expectedCount, cbCount.to_i]
            end 
        else
            return cbCount.to_i
        end
    end

    def clusterNodeGetAsyncInvocation(resource, expectedInvocation)
        kvpHash = {'CLM_RESOURCE_ID' => resource.getID()}
        kvpResults = runDriver('CLUSTER_NODE_GET_ASYNC_INVOCATION_REQ', kvpHash,
                               SAFTestUtils.SA_AIS_OK)
        ret = array[0]
        lines = array[1]
        invocation = kvpResults['CLUSTER_NODE_GET_ASYNC_INVOCATION'].to_i
        if invocation != expectedInvocation
            raise "Expected invocation %d, got %d." % \
                  [expectedInvocation, invocation]
        end
    end

    def clusterTrack(resource, track_current, track_changes, 
                     track_changes_only, invalid_track_flags,
                     null_notification_buffer, null_cluster_notification,
                     number_of_items, xmlFile, expectedReturn)
        kvpHash = {'CLM_RESOURCE_ID' => resource.getID(),
                    'TRACK_CURRENT' => 'FALSE',
                    'TRACK_CHANGES' => 'FALSE',
                    'TRACK_CHANGES_ONLY' => 'FALSE',
                    'INVALID_TRACK_FLAGS' => 'FALSE',
                    'NULL_NOTIFICATION_BUFFER' => 'FALSE',
                    'NULL_CLUSTER_NOTIFICATION' => 'FALSE',
                    'NUMBER_OF_ITEMS' => number_of_items}
        if track_current
            kvpHash['TRACK_CURRENT'] = 'TRUE'
        end
        if track_changes
            kvpHash['TRACK_CHANGES'] = 'TRUE'
        end
        if track_changes_only
            kvpHash['TRACK_CHANGES_ONLY'] = 'TRUE'
        end
        if invalid_track_flags
            kvpHash['INVALID_TRACK_FLAGS'] = 'TRUE'
        end
        if null_notification_buffer
            kvpHash['NULL_NOTIFICATION_BUFFER'] = 'TRUE'
        end
        if null_cluster_notification
            kvpHash['NULL_CLUSTER_NOTIFICATION'] = 'TRUE'
        end
        if xmlFile != nil
            kvpHash['XML_FILE'] = xmlFile
        end

        runDriver("CLUSTER_TRACK_REQ", kvpHash, expectedReturn)
    end

    def clusterTrackStop(resource, expectedReturn)
        kvpHash = {'CLM_RESOURCE_ID' => resource.getID()}
        array = runDriver('CLUSTER_TRACK_STOP_REQ', kvpHash,
                          expectedReturn)
    end

    def displayLastNotificationBuffer(resource, xmlFile, expectedReturn)
        kvpHash = {'CLM_RESOURCE_ID' => resource.getID(),
                    'XML_FILE' => xmlFile}
        array = runDriver('DISPLAY_LAST_NOTIFICATION_BUFFER_REQ', kvpHash,
                          SAFTestUtils.SA_AIS_OK)
    end
end # class

end # module
