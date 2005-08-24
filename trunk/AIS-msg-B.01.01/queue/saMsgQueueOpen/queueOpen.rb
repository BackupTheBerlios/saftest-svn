#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

msgDir = "%s/AIS-msg-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << msgDir

class QueueOpenCase < Test::Unit::TestCase
    require 'MSGTestDriver'
    @@SIZE_ARRAY = [65535, 65535, 65535, 65535]

    def test_run()
        driver = MSGTestDriver::MSGTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.init(resourceID, "SA_DISPATCH_ALL",
                    SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.selectObjectGet(resourceID, false,
                               SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.queueOpen(resourceID, "queue1", false, @@SIZE_ARRAY, 0,
                         true, false, false,
                         SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.stop()
    end
end
