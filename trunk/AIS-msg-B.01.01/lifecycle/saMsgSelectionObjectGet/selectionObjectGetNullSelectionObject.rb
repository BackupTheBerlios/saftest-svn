#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

msgDir = "%s/AIS-msg-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << msgDir

class SelectionObjectGetNullSelectionObjectCase < Test::Unit::TestCase
    require 'MSGTestDriver'

    def test_run()
        driver = MSGTestDriver::MSGTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.init(resourceID, "SA_DISPATCH_ALL",
                    SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.selectObjectGet(resourceID, true,
                               SAFTestUtils::SAFTestUtils.SA_AIS_ERR_INVALID_PARAM)
        driver.stop()
    end
end
