#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

msgDir = "%s/AIS-msg-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << msgDir

class MessageGetAndReplyCase < Test::Unit::TestCase
    require 'MSGTestDriver'
    @@SIZE_ARRAY = [65535, 65535, 65535, 65535]
    @@QUEUE_NAME = 'queue1'
    @@SENDER_NAME = 'SAF_MSG_Driver'
    @@MSG_TYPE = 1
    @@MSG_VERSION = 1
    @@MSG_PRIORITY = 1
    @@MSG_STRING = 'Hello World'
    @@MSG_REPLY_STRING = 'Goodbye Cruel World'

    def test_run()
        driver = MSGTestDriver::MSGTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.init(resourceID, "SA_DISPATCH_ALL",
                    SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.selectObjectGet(resourceID, false,
                               SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.queueOpen(resourceID, @@QUEUE_NAME, false, @@SIZE_ARRAY, 0,
                         true, false, false,
                         SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.messageSend(resourceID, @@QUEUE_NAME, @@SENDER_NAME,
                           @@MSG_TYPE, @@MSG_VERSION, @@MSG_PRIORITY,
                           @@MSG_STRING, SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.messageGet(resourceID, @@QUEUE_NAME, @@SENDER_NAME,
                          @@MSG_TYPE, @@MSG_VERSION, @@MSG_PRIORITY,
                          @@MSG_STRING, SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.stop()
    end
end
