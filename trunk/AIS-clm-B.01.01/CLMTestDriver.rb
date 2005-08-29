module CLMTestDriver

require 'SAFTestDriver'
require 'SAFTestUtils'

class CLMTestDriver < SAFTestDriver::SAFTestDriver
    @@nextInstanceID = 1

    def initialize(node)
        driverLib = "%s/AIS-clm-%s/driver/clm_driver.so" % \
                    [ENV['SAFTEST_ROOT'], 
                     SAFTestUtils::SAFTestUtils.getAISLibVersion()]
        instanceID = @@nextInstanceID
        @@nextInstanceID += 1
        super(node, driverLib, instanceID)
    end

    def getName()
        return 'clm_driver'
    end

    def createTestResource()
        array = runDriver("CREATE_TEST_RESOURCE_REQ", {}, 
                          SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        ret = array[0]
        lines = array[1]
        resourceID = nil
        lines.each do |line|
            if line =~ /^Resource ID=(\d+)/
                resourceID = $1
            end
        end
        if nil == resourceID 
            raise "Couldn't find a Resource ID.  Lines = \"%s\"" % [lines.to_s]
        end
        log("new resourceID is %d" % [resourceID])
        return resourceID
    end

    def init(resourceID, setClusterNodeGetCB, setClusterTrackCB,
             dispatchFlags, expectedReturn)
        kvp_hash = {'CLM_RESOURCE_ID' => resourceID,
                    'DISPATCH_FLAGS' => dispatchFlags,
                    'CLUSTER_NODE_GET_CB' => 'FALSE',
                    'CLUSTER_TRACK_CB' => 'FALSE',
                    'VERSION_RELEASE_CODE' =>
                        SAFTestUtils::SAFTestUtils.SA_AIS_RELEASE_CODE,
                    'VERSION_MAJOR' =>
                        SAFTestUtils::SAFTestUtils.SA_AIS_MAJOR_VERSION,
                    'VERSION_MINOR' =>
                        SAFTestUtils::SAFTestUtils.SA_AIS_MINOR_VERSION,
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

    def initWithOptions(resourceID, dispatchFlags, 
                        releaseCode, majorVersion, minorVersion,
                        nullClmHandle, nullCallbacks, nullVersion,
                        expectedReturn)
        kvp_hash = {'CLM_RESOURCE_ID' => resourceID,
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

    def finalize(resourceID, expectedReturn)
        kvp_hash = {'CLM_RESOURCE_ID' => resourceID}
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

    def clusterNodeGet(resourceID, nodeIDString, timeout, nullClusterNode,
                       expectedReturn)
        kvp_hash = {'CLM_RESOURCE_ID' => resourceID,
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

    def clusterNodeGetCBCount(resourceID, expectedCount)
        kvp_hash = {'CLM_RESOURCE_ID' => resourceID}
        array = runDriver('CLUSTER_NODE_GET_CALLBACK_COUNT_REQ', kvp_hash,
                          SAFTestUtils::SAFTestUtils.SA_AIS_OK)
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
        if cbCount != expectedCount.to_s
            raise "Expected count %d, got %s.  Lines = \"%s\"" % \
                  [expectedCount, cbCount, lines.to_s]
        end
    end

    def clusterTrackCBCount(resourceID, expectedCount)
        kvp_hash = {'CLM_RESOURCE_ID' => resourceID}
        array = runDriver('CLUSTER_TRACK_CALLBACK_COUNT_REQ', kvp_hash,
                          SAFTestUtils::SAFTestUtils.SA_AIS_OK)
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
        if cbCount != expectedCount.to_s
            raise "Expected count %d, got %s.  Lines = \"%s\"" % \
                  [expectedCount, cbCount, lines.to_s]
        end
    end

    def clusterNodeGetAsyncInvocation(resourceID, expectedInvocation)
        kvp_hash = {'CLM_RESOURCE_ID' => resourceID}
        array = runDriver('CLUSTER_NODE_GET_ASYNC_INVOCATION_REQ', kvp_hash,
                          SAFTestUtils::SAFTestUtils.SA_AIS_OK)
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
                          SAFTestUtils::SAFTestUtils.SA_AIS_OK)
    end
end # class

end # module
