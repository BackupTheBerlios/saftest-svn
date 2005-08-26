#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

msgDir = "%s/AIS-msg-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << msgDir

class MessageSendAndGetNonPersistentDifferentNodes < Test::Unit::TestCase
    require 'MSGTestDriver'
    @@SIZE_ARRAY = [65535, 65535, 65535, 65535]
    @@QUEUE_NAME = 'queue1'
    @@SENDER_NAME = 'SAF_MSG_Driver'
    @@MSG_TYPE = 1
    @@MSG_VERSION = 1
    @@MSG_PRIORITY = 1
    @@MSG_STRING_1 = 'Hello World!'
    @@MSG_STRING_2 = 'Goodbye Cruel World!'
    @@MSG_STRING_3 = 'Hello Again World!'

    def test_run()
        driver1 = MSGTestDriver::MSGTestDriver.new(nil)
        driver1.killAllDrivers()
        impl = driver1.getImplementation()
        cluster = impl.getClusterFromCommand()
        impl.ensureAllNodesAreUp(cluster)

        if cluster.getNodes().length < 2
            driver1.log("Need at least 2 nodes to continue, only have %d" % \
                       [cluster.getNodes().length])
        else
            upNode = nil
            cluster.getNodes().each do |node|
                if !node.isLocalNode()
                    upNode = node
                    break
                end
            end
            driver2 = MSGTestDriver::MSGTestDriver.new(upNode)
            driver2.killAllDrivers()
            driver2.start()
            resource2ID = driver2.createTestResource()
            driver2.init(resource2ID, "SA_DISPATCH_ALL",
                        SAFTestUtils::SAFTestUtils.SA_AIS_OK)
            driver2.selectObjectGet(resource2ID, false,
                                   SAFTestUtils::SAFTestUtils.SA_AIS_OK)
            driver2.messageSend(resource2ID, @@QUEUE_NAME, @@SENDER_NAME,
                                @@MSG_TYPE, @@MSG_VERSION, @@MSG_PRIORITY,
                                @@MSG_STRING_1, 
                                SAFTestUtils::SAFTestUtils.SA_AIS_OK)
            driver2.messageSend(resource2ID, @@QUEUE_NAME, @@SENDER_NAME,
                                @@MSG_TYPE, @@MSG_VERSION, @@MSG_PRIORITY,
                                @@MSG_STRING_2, 
                                SAFTestUtils::SAFTestUtils.SA_AIS_OK)
            driver2.queueOpen(resource2ID, "queue1", false, @@SIZE_ARRAY, 0,
                              true, false, false,
                              SAFTestUtils::SAFTestUtils.SA_AIS_OK)
            driver2.messageGet(resource2ID, @@QUEUE_NAME, @@SENDER_NAME,
                               @@MSG_TYPE, @@MSG_VERSION, @@MSG_PRIORITY,
                               @@MSG_STRING_1, SAFTestUtils::SAFTestUtils.SA_AIS_OK)
            driver2.queueClose(resource2ID,
                               SAFTestUtils::SAFTestUtils.SA_AIS_OK)
            driver2.messageSend(resource2ID, @@QUEUE_NAME, @@SENDER_NAME,
                                @@MSG_TYPE, @@MSG_VERSION, @@MSG_PRIORITY,
                                @@MSG_STRING_3, 
                                SAFTestUtils::SAFTestUtils.SA_AIS_OK)

            driver1.start()
            resource1ID = driver1.createTestResource()
            driver1.init(resource1ID, "SA_DISPATCH_ALL",
                        SAFTestUtils::SAFTestUtils.SA_AIS_OK)
            driver1.selectObjectGet(resource1ID, false,
                                   SAFTestUtils::SAFTestUtils.SA_AIS_OK)
            driver1.queueOpen(resource1ID, "queue1", false, @@SIZE_ARRAY, 0,
                             true, false, false,
                             SAFTestUtils::SAFTestUtils.SA_AIS_OK)
            driver1.messageGet(resource1ID, @@QUEUE_NAME, @@SENDER_NAME,
                               @@MSG_TYPE, @@MSG_VERSION, @@MSG_PRIORITY,
                               @@MSG_STRING_3, SAFTestUtils::SAFTestUtils.SA_AIS_OK)
            driver1.queueClose(resource1ID,
                               SAFTestUtils::SAFTestUtils.SA_AIS_OK)

            driver1.stop()
            driver2.stop()
        end
    end
end
