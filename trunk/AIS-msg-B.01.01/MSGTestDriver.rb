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
        cmd = "%s --run-dir %s --o CREATE_TEST_RESOURCE_REQ --load-libs %s --socket-file %s" % \
              [getDriverPath(), getRunDir(), getDriverLibs(), getSocketFile()]
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

    def runDriver(cmd, kvp_hash, expectedReturn)
        newCmd = "%s --run-dir %s --socket-file %s --load-libs %s %s" % \
                 [getDriverPath(), getRunDir(), getSocketFile(),
                  getDriverLibs(), cmd]
        kvp_hash.each do |key, value|
            newCmd = "%s --key \"%s\" --value \"%s\"" % [newCmd, key, value]
        end
        array = captureCommand(newCmd)
        ret = array[0]
        lines = array[1]
        lines.each do |line|
            print line
        end
        if expectedReturn != ret
            raise "Expected return %s, got %s.  Lines = \"%s\"" % \
                   [mapErrorCodeToString(expectedReturn), 
                    mapErrorCodeToString(ret), lines.to_s]
        end
    end

    def init(resourceID, dispatchFlags, expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s,
                    'QUEUE_OPEN_CB' => 'TRUE',
                    'QUEUE_GROUP_TRACK_CB' => 'TRUE',
                    'MESSAGE_DELIVERED_CB' => 'TRUE',
                    'MESSAGE_RECEIEVED_CB' => 'TRUE',
                    'VERSION_RELEASE_CODE' => 'B',
                    'VERSION_MAJOR' => '1',
                    'VERSION_MINOR' => '1',
                    'DISPATCH_FLAGS' => dispatchFlags,
                    'NULL_MSG_HANDLE' => 'FALSE',
                    'NULL_CALLBACKS' => 'FALSE',
                    'NULL_VERSION' => 'FALSE'}
        cmd = "--o MSG_INITIALIZE_REQ"
        runDriver("--o MSG_INITIALIZE_REQ", kvp_hash, expectedReturn)
    end

    def initWithOptions(resourceID, dispatchFlags, 
                        releaseCode, majorVersion, minorVersion,
                        nullMsgHandle, nullCallbacks, nullVersion,
                        expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s,
                    'VERSION_RELEASE_CODE' => releaseCode,
                    'VERSION_MAJOR' => majorVersion,
                    'VERSION_MINOR' => minorVersion,
                    'DISPATCH_FLAGS' => dispatchFlags}

        if nullCallbacks
            kvp_hash['NULL_CALLBACKS'] = 'TRUE'
        else
            kvp_hash['NULL_CALLBACKS'] = 'FALSE'
            kvp_hash['QUEUE_OPEN_CB'] = 'TRUE'
            kvp_hash['QUEUE_GROUP_TRACK_CB'] = 'TRUE'
            kvp_hash['MESSAGE_DELIVERED_CB'] = 'TRUE'
            kvp_hash['MESSAGE_RECEIEVED_CB'] = 'TRUE'
        end

        if nullMsgHandle
            kvp_hash['NULL_MSG_HANDLE'] = 'TRUE'
        else
            kvp_hash['NULL_MSG_HANDLE'] = 'FALSE'
        end

        if nullVersion
            kvp_hash['NULL_VERSION'] = 'TRUE'
        else
            kvp_hash['NULL_VERSION'] = 'FALSE'
        end

        runDriver("--o MSG_INITIALIZE_REQ", kvp_hash, expectedReturn)
    end

    def dispatch(resourceID, dispatchFlags, expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s,
                    'DISPATCH_FLAGS' => dispatchFlags}
        runDriver("--o DISPATCH_REQ", kvp_hash, expectedReturn)
    end

    def finalize(resourceID, expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s}
        runDriver("--o FINALIZE_REQ", kvp_hash, expectedReturn)
    end

    def selectObjectGet(resourceID, nullSelectionObject, expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s,
                    'NULL_SELECTION_OBJECT' => 'FALSE'}
        if nullSelectionObject
            kvp_hash['NULL_SELECTION_OBJECT'] = 'TRUE'
        end
        runDriver("--o SELECTION_OBJECT_GET_REQ", kvp_hash, expectedReturn)
    end

    def queueOpen(resourceID, queueName, persistent, sizeArray, retentionTime,
                  create, receiveCallback, empty, expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s,
                    'QUEUE_NAME' => queueName,
                    'PERSISTENT' => 'FALSE',
                    'CREATE' => 'FALSE',
                    'EMPTY' => 'FALSE',
                    'RECEIVE_CALLBACK' => 'FALSE',
                    'RETENTION_TIME' => retentionTime.to_s}
        sizeNdx = 0
        sizeArray.each do |size|
            kvp_hash["SIZE_ARRAY_%d" % [sizeNdx]] = size
            sizeNdx += 1
        end    
        if persistent
            if 0 != retentionTime
                raise "Retention Time only used for non-persistent queues"
            end
            kvp_hash['PERSISTENT'] = 'TRUE'
        end
        if create
            kvp_hash['CREATE'] = 'TRUE'
        end
        if receiveCallback
            kvp_hash['RECEIVE_CALLBACK'] = 'TRUE'
        end
        if empty
            kvp_hash['EMPTY'] = 'TRUE'
        end
        runDriver("--o QUEUE_OPEN_REQ", kvp_hash, expectedReturn)
    end

    def queueClose(resourceID, expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s}
        runDriver("--o QUEUE_CLOSE_REQ", kvp_hash, expectedReturn)
    end

    def messageSend(resourceID, entityName, senderName, msgType, msgVersion,
                    msgPriority, msgString, expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s,
                    'ENTITY_NAME' => entityName,
                    'SENDER_NAME' => senderName,
                    'MSG_TYPE' => msgType.to_s,
                    'MSG_VERSION' => msgVersion.to_s,
                    'MSG_PRIORITY' => msgPriority.to_s,
                    'MSG_STRING' => msgString}
        runDriver("--o MESSAGE_SEND_REQ", kvp_hash, expectedReturn)
    end

    def messageGet(resourceID, entityName, expectedSenderName, 
                   expectedMsgType, expectedMsgVersion,
                   expectedMsgPriority, expectedMsgString, expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s,
                    'ENTITY_NAME' => entityName}
        runDriver("--o MESSAGE_GET_REQ", kvp_hash, expectedReturn)
    end

    def messageSendReceive(resourceID, entityName, senderName, 
                           msgType, msgVersion, msgPriority, msgString, 
                           expectedMsgType, expectedMsgVersion,
                           expectedMsgPriority, expectedMsgString, 
                           expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s,
                    'ENTITY_NAME' => entityName,
                    'SENDER_NAME' => senderName,
                    'MSG_TYPE' => msgType.to_s,
                    'MSG_VERSION' => msgVersion.to_s,
                    'MSG_PRIORITY' => msgPriority.to_s,
                    'MSG_STRING' => msgString}
        runDriver("--o MESSAGE_SEND_RECEIVE_REQ", kvp_hash, expectedReturn)

    def messageReply(resourceID, entityName, senderName, 
                     msgType, msgVersion, msgPriority, replyString, 
                     expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s,
                    'ENTITY_NAME' => entityName,
                    'SENDER_NAME' => senderName,
                    'MSG_TYPE' => msgType.to_s,
                    'MSG_VERSION' => msgVersion.to_s,
                    'MSG_PRIORITY' => msgPriority.to_s,
                    'REPLY_STRING' => replyString}
        runDriver("--o MESSAGE_REPLY_REQ", kvp_hash, expectedReturn)
    end

end # class

end # module
