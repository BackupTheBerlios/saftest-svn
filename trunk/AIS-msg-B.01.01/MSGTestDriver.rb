module MSGTestDriver

require 'SAFTestDriver'
require 'SAFTestUtils'

class MSGTestDriver < SAFTestDriver::SAFTestDriver
    @@nextInstanceID = 1

    def initialize(node)
        driverLib = "%s/AIS-msg-%s/driver/msg_driver.so" % \
                    [ENV['SAFTEST_ROOT'],
                     SAFTestUtils::SAFTestUtils.getAISLibVersion()]
        instanceID = @@nextInstanceID
        @@nextInstanceID += 1
        super(node, driverLib, instanceID)
    end

    def getName()
        return 'msg_driver'
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
        cmd = "--o INIT --resource-id %s --set-queue-open-cb --set-queue-group-track-cb --set-message-delivered-cb --set-message-received-cb --dispatch-flags %s" % [resourceID, dispatchFlags]
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
            cmd += " --null-msg-handle"
        end
        if nullCallbacks
            cmd += " --null-callbacks"
        end
        if nullVersion
            cmd += " --null-version"
        end
        runDriver(cmd, expectedReturn)
    end

    def dispatch(resourceID, dispatchFlags, expectedReturn)
        cmd = "--o DISPATCH --resource-id %s --dispatch-flags %s" % \
              [resourceID, dispatchFlags]
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

    def queueOpen(resourceID, queue_name, persistent, sizeArray, retentionTime,
                  create, receiveCallback, empty, expectedReturn)
        cmd = "--o QUEUE_OPEN --resource-id %s --queue-name %s --retention-time %d --size-array " % [resourceID, queue_name, retentionTime]
        sizeArray.each do |size|
            cmd += "%d," % [size]
        end    
        if persistent
            if 0 != retentionTime
                raise "Retention Time only used for non-persistent queues"
            end
            cmd += " --persistent"
        end
        if create
            cmd += " --create"
        end
        if receiveCallback
            cmd += " --receive-callback"
        end
        if empty
            cmd += " --empty"
        end
        runDriver(cmd, expectedReturn)
    end

end # class

end # module
