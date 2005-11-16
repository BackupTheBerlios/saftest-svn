module SAFTest

require 'SAFTestDriver'
require 'SAFTestUtils'

class CLMTestResource < SAFTestUtils
    def initialize(resourceID)
        @resourceID = resourceID
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
end

class CLMTestDriver < SAFTestDriver
    @@nextInstanceID = 1

    def initialize(node=nil, instanceID=0)
        driverLib = "%s/cases/clm/driver/clm_driver.so" % \
                    [ENV['SAFTEST_ROOT']]
        super(node, driverLib, instanceID)
    end

    def getName()
        return 'clm_driver'
    end

    def createTestResource()
        results = runDriver("CREATE_TEST_RESOURCE_REQ", {}, 
                                SAFTestUtils.SA_AIS_OK)

        results.each do |key, value|
            print "#{key}=#{value}\n"
        end
        resourceID = results["CLM_RESOURCE_ID"].to_i
        resource = CLMTestResource.new(resourceID)
        return resource
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
            resourceArray << resource
        end
        return resourceArray
    end

    def getRandomTestResource()
        resourceArray = getAllTestResources()
        return resourceArray[rand(resourceArray.length())]
    end

    def init(resource, setClusterNodeGetCB, setClusterTrackCB,
             dispatchFlags, expectedReturn)
        kvp_hash = {'CLM_RESOURCE_ID' => resource.getID(),
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
            kvp_hash['CLUSTER_NODE_GET_CB'] = 'TRUE'
        end
        if setClusterTrackCB
            kvp_hash['CLUSTER_TRACK_CB'] = 'TRUE'
        end
        runDriver("INITIALIZE_REQ", kvp_hash, expectedReturn)
    end

    def initWithOptions(resource, dispatchFlags, 
                        releaseCode, majorVersion, minorVersion,
                        nullClmHandle, nullCallbacks, nullVersion,
                        expectedReturn)
        kvp_hash = {'CLM_RESOURCE_ID' => resource.getID(),
                    'VERSION_RELEASE_CODE' => releaseCode,
                    'VERSION_MAJOR' => majorVersion,
                    'VERSION_MINOR' => minorVersion,
                    'NULL_CLM_HANDLE' => 'FALSE',
                    'NULL_VERSION' => 'FALSE',
                    'DISPATCH_FLAGS' => dispatchFlags}

        if nullClmHandle
            kvp_hash['NULL_CLM_HANDLE'] = 'TRUE'
        end

        if nullCallbacks
            kvp_hash['NULL_CALLBACKS'] = 'TRUE'
        else
            kvp_hash['NULL_CALLBACKS'] = 'FALSE'
            kvp_hash['CLUSTER_NODE_GET_CB'] = 'TRUE'
            kvp_hash['CLUSTER_TRACK_CB'] = 'TRUE'
        end

        if nullVersion
            kvp_hash['NULL_VERSION'] = 'TRUE'
        end

        runDriver("INITIALIZE_REQ", kvp_hash, expectedReturn)
    end

    def finalize(resource, expectedReturn)
        kvp_hash = {'CLM_RESOURCE_ID' => resource.getID()}
        runDriver("FINALIZE_REQ", kvp_hash, expectedReturn)
    end

    def selectObjectGet(resourceID, nullSelectionObject, expectedReturn)
        kvp_hash = {'CLM_RESOURCE_ID' => resourceID,
                    'NULL_SELECTION_OBJECT' => 'FALSE'}
        if nullSelectionObject
            kvp_hash['NULL_SELECTION_OBJECT'] = 'TRUE'
        end
        runDriver("SELECTION_OBJECT_GET_REQ", kvp_hash, expectedReturn)

    end

    def dispatch(resourceID, dispatchFlags, expectedReturn)
        kvp_hash = {'CLM_RESOURCE_ID' => resourceID,
                    'DISPATCH_FLAGS' => dispatchFlags}
        runDriver("DISPATCH_REQ", kvp_hash, expectedReturn)
    end

    def clusterNodeGet(resource, nodeIDString, timeout, nullClusterNode,
                       expectedReturn)
        kvp_hash = {'CLM_RESOURCE_ID' => resource.getID(),
                    'NODE_ID' => nodeIDString,
                    'NULL_CLUSTER_NODE' => 'FALSE',
                    'TIMEOUT' => timeout}
        if nullClusterNode
            kvp_hash['NULL_CLUSTER_NODE'] = 'TRUE'
        end
        runDriver("CLUSTER_NODE_GET_REQ", kvp_hash, expectedReturn)
    end

    def generateInvocation()
        return rand(1000)
    end

    def clusterNodeGetAsync(resourceID, invocation, nodeIDString, 
                            expectedReturn)
        if invocation <= 0
            raise "invocation must be greater than 0"
        end
        kvp_hash = {'CLM_RESOURCE_ID' => resourceID,
                    'INVOCATION' => invocation,
                    'NODE_ID' => nodeIDString}
        runDriver("CLUSTER_NODE_GET_ASYNC_REQ", kvp_hash, expectedReturn)
    end

    def clusterNodeGetCBCount(resourceID)
        kvp_hash = {'CLM_RESOURCE_ID' => resourceID}
        array = runDriver('CLUSTER_NODE_GET_CALLBACK_COUNT_REQ', kvp_hash,
                          SAFTestUtils.SA_AIS_OK)
        ret = array[0]
        lines = array[1]
        cbCount = nil
        lines.each do |line|
            if line =~ /^Cluster Node Get Callback Count=(\d+)/
                cbCount = $1
            end
        end
        if nil == cbCount
            raise "Couldnt find a Cluster Node Get Callback Count.  Lines = \"%s\"" % [lines.to_s]
        end
        return cbCount.to_i
    end

    def clusterTrackCBCount(resourceID)
        kvp_hash = {'CLM_RESOURCE_ID' => resourceID}
        array = runDriver('CLUSTER_TRACK_CALLBACK_COUNT_REQ', kvp_hash,
                          SAFTestUtils.SA_AIS_OK)
        ret = array[0]
        lines = array[1]
        cbCount = nil
        lines.each do |line|
            if line =~ /^Cluster Track Callback Count=(\d+)/
                cbCount = $1
            end
        end
        if nil == cbCount
            raise "Couldnt find a Cluster Track Callback Count.  Lines = \"%s\"" % [lines.to_s]
        end
        return cbCount.to_i
    end

    def clusterNodeGetAsyncInvocation(resourceID, expectedInvocation)
        kvp_hash = {'CLM_RESOURCE_ID' => resourceID}
        array = runDriver('CLUSTER_NODE_GET_ASYNC_INVOCATION_REQ', kvp_hash,
                          SAFTestUtils.SA_AIS_OK)
        ret = array[0]
        lines = array[1]
        invocation = nil
        lines.each do |line|
            if line =~ /^Cluster Node Get Async Invocation=(\d+)/
                invocation = $1
            end
        end
        if nil == invocation
            raise "Couldnt find a Cluster Node Get Async Invocation.  Lines = \"%s\"" % [lines.to_s]
        end
        if invocation != expectedInvocation.to_s
            raise "Expected invocation %d, got %d.  Lines = \"%s\"" % \
                  [expectedInvocation, invocation, lines.to_s]
        end
    end

    def clusterTrack(resourceID, track_current, track_changes, 
                     track_changes_only, invalid_track_flags,
                     null_notification_buffer, null_cluster_notification,
                     number_of_items, expectedReturn)
        kvp_hash = {'CLM_RESOURCE_ID' => resourceID,
                    'TRACK_CURRENT' => 'FALSE',
                    'TRACK_CHANGES' => 'FALSE',
                    'TRACK_CHANGES_ONLY' => 'FALSE',
                    'INVALID_TRACK_FLAGS' => 'FALSE',
                    'NULL_NOTIFICATION_BUFFER' => 'FALSE',
                    'NULL_CLUSTER_NOTIFICATION' => 'FALSE',
                    'NUMBER_OF_ITEMS' => number_of_items}
        if track_current
            kvp_hash['TRACK_CURRENT'] = 'TRUE'
        end
        if track_changes
            kvp_hash['TRACK_CHANGES'] = 'TRUE'
        end
        if track_changes_only
            kvp_hash['TRACK_CHANGES_ONLY'] = 'TRUE'
        end
        if invalid_track_flags
            kvp_hash['INVALID_TRACK_FLAGS'] = 'TRUE'
        end
        if null_notification_buffer
            kvp_hash['NULL_NOTIFICATION_BUFFER'] = 'TRUE'
        end
        if null_cluster_notification
            kvp_hash['NULL_CLUSTER_NOTIFICATION'] = 'TRUE'
        end
        runDriver("CLUSTER_TRACK_REQ", kvp_hash, expectedReturn)
    end

    def clusterTrackStop(resourceID, expectedReturn)
        kvp_hash = {'CLM_RESOURCE_ID' => resourceID}
        array = runDriver('CLUSTER_TRACK_STOP_REQ', kvp_hash,
                          expectedReturn)
    end

    def displayLastNotificationBuffer(resourceID, xmlFile, expectedReturn)
        kvp_hash = {'CLM_RESOURCE_ID' => resourceID,
                    'XML_FILE' => xmlFile}
        array = runDriver('DISPLAY_LAST_NOTIFICATION_BUFFER_REQ', kvp_hash,
                          SAFTestUtils.SA_AIS_OK)
    end
end # class

end # module
