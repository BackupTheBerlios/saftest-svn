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
            raise "Couldn't find a Resource ID.  Lines = \"%s\"" % [lines.to_s]
        end
        log("new resourceID is %d" % [resourceID])
        return resourceID
    end

    def runDriver(cmd, expectedReturn)
        newCmd = '%s --run-dir %s --socket-file %s --load-libs %s %s' % \
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

    def init(resourceID, setClusterNodeGetCB, setClusterTrackCB,
             dispatchFlags, expectedReturn)
        cmd = "--o INIT --resource-id %s --dispatch-flags %s" \
              % [resourceID, dispatchFlags]
        if setClusterNodeGetCB
            cmd += " --set-cluster-node-get-cb"
        end
        if setClusterTrackCB
            cmd += " --set-cluster-track-cb"
        end

        runDriver(cmd, expectedReturn)
    end

    def initWithOptions(resourceID, dispatchFlags, 
                        releaseCode, majorVersion, minorVersion,
                        nullClmHandle, nullCallbacks, nullVersion,
                        expectedReturn)
        cmd = "--o INIT --resource-id %s --dispatch-flags %s --version-release-code %d --version-major %d --version-minor %d" % \
              [resourceID, 
               dispatchFlags, releaseCode, majorVersion, minorVersion]
        if nullClmHandle
            cmd += " --null-clm-handle"
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

    def dispatch(resourceID, dispatchFlags, expectedReturn)
        cmd = "--o DISPATCH --resource-id %s --dispatch-flags %s" % \
            [resourceID, dispatchFlags]
        runDriver(cmd, expectedReturn)
    end

    def clusterNodeGet(resourceID, nodeIDString, timeout, nullClusterNode,
                       expectedReturn)
        cmd = "--o CLUSTER_NODE_GET --resource-id %s --node-id %s --timeout %d" % \
              [resourceID, nodeIDString, timeout]
        if nullClusterNode
            cmd += " --null-cluster-node"
        end
        runDriver(cmd, expectedReturn)
    end

    def generateInvocation()
        return rand(1000)
    end

    def clusterNodeGetAsync(resourceID, invocation, nodeIDString, 
                            expectedReturn)
        if invocation <= 0
            raise "invocation must be greater than 0"
        end

        cmd = "--o CLUSTER_NODE_GET_ASYNC --resource-id %s --invocation %d --node-id %s" % \
              [resourceID, invocation, nodeIDString]
        runDriver(cmd, expectedReturn)
    end

    def clusterNodeGetCBCount(resourceID, expectedCount)
        cmd = "%s --run-dir %s --socket-file %s --load-libs %s --o CLUSTER_NODE_GET_CALLBACK_COUNT --resource-id %s" % \
              [getDriverPath(), getRunDir(), getSocketFile(), getDriverLibs(),
               resourceID]
        array = captureCommand(cmd)
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
        cmd = "%s --run-dir %s --socket-file %s --load-libs %s --o CLUSTER_TRACK_CALLBACK_COUNT --resource-id %s" % \
              [getDriverPath(), getRunDir(), getSocketFile(), getDriverLibs(),
               resourceID]
        array = captureCommand(cmd)
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
        cmd = "%s --run-dir %s --socket-file %s --load-libs %s --o CLUSTER_NODE_GET_ASYNC_INVOCATION --resource-id %s" % \
              [getDriverPath(), getRunDir(), getSocketFile(), getDriverLibs(),
               resourceID]
        array = captureCommand(cmd)
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
        cmd = "--o CLUSTER_TRACK --resource-id %s" % [resourceID]
        if track_current
            cmd += " --track-current"
        end
        if track_changes
            cmd += " --track-changes"
        end
        if track_changes_only
            cmd += " --track-changes-only"
        end
        if invalid_track_flags
            cmd += " --invalid-track-flags"
        end
        if null_notification_buffer
            cmd += " --null-notification-buffer"
        end
        if null_cluster_notification
            cmd += " --null-cluster-notification"
        end
        if !null_notification_buffer and number_of_items >= 0
            cmd += " --number-of-items %d" % [number_of_items]
        end
        runDriver(cmd, expectedReturn)
    end

    def clusterTrackStop(resourceID, expectedReturn)
        cmd = "--o CLUSTER_TRACK_STOP --resource-id %s" % [resourceID]
        runDriver(cmd, expectedReturn)
    end

    def displayLastNotificationBuffer(resourceID, xmlFile, expectedReturn)
        cmd = "--o DISPLAY_LAST_NOTIFICATION_BUFFER --resource-id %s --xml-file %s" % [resourceID, xmlFile]
        runDriver(cmd, expectedReturn)
    end
end # class

end # module
