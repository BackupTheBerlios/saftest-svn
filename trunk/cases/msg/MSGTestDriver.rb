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
        #log("new resourceID is %d" % [resourceID])
        return resourceID
    end

    def init(resourceID, dispatchFlags, expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s,
                    'QUEUE_OPEN_CB' => 'TRUE',
                    'QUEUE_GROUP_TRACK_CB' => 'TRUE',
                    'MESSAGE_DELIVERED_CB' => 'TRUE',
                    'MESSAGE_RECEIEVED_CB' => 'TRUE',
                    'VERSION_RELEASE_CODE' =>
                        SAFTestUtils::SAFTestUtils.SA_AIS_RELEASE_CODE,
                    'VERSION_MAJOR' =>
                        SAFTestUtils::SAFTestUtils.SA_AIS_MAJOR_VERSION,
                    'VERSION_MINOR' =>
                        SAFTestUtils::SAFTestUtils.SA_AIS_MINOR_VERSION,
                    'DISPATCH_FLAGS' => dispatchFlags,
                    'NULL_MSG_HANDLE' => 'FALSE',
                    'NULL_CALLBACKS' => 'FALSE',
                    'NULL_VERSION' => 'FALSE'}
        runDriver("INITIALIZE_REQ", kvp_hash, expectedReturn)
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

        runDriver("INITIALIZE_REQ", kvp_hash, expectedReturn)
    end

    def dispatch(resourceID, dispatchFlags, expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s,
                    'DISPATCH_FLAGS' => dispatchFlags}
        runDriver("DISPATCH_REQ", kvp_hash, expectedReturn)
    end

    def finalize(resourceID, expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s}
        runDriver("FINALIZE_REQ", kvp_hash, expectedReturn)
    end

    def selectObjectGet(resourceID, nullSelectionObject, expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s,
                    'NULL_SELECTION_OBJECT' => 'FALSE'}
        if nullSelectionObject
            kvp_hash['NULL_SELECTION_OBJECT'] = 'TRUE'
        end
        runDriver("SELECTION_OBJECT_GET_REQ", kvp_hash, expectedReturn)
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
        runDriver("QUEUE_OPEN_REQ", kvp_hash, expectedReturn)
    end

    def queueClose(resourceID, expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s}
        runDriver("QUEUE_CLOSE_REQ", kvp_hash, expectedReturn)
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
        runDriver("MESSAGE_SEND_REQ", kvp_hash, expectedReturn)
    end

    def messageGet(resourceID, entityName, expectedSenderName, 
                   expectedMsgType, expectedMsgVersion,
                   expectedMsgPriority, expectedMsgString, expectedReturn)
        kvp_hash = {'MSG_RESOURCE_ID' => resourceID.to_s,
                    'ENTITY_NAME' => entityName}
        runDriver("MESSAGE_GET_REQ", kvp_hash, expectedReturn)
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
        runDriver("MESSAGE_SEND_RECEIVE_REQ", kvp_hash, expectedReturn)
    end

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
        runDriver("MESSAGE_REPLY_REQ", kvp_hash, expectedReturn)
    end

end # class

end # module
