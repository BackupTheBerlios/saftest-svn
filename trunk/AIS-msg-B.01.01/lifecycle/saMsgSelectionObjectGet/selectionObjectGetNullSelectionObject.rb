#!/usr/bin/ruby

$: << "%s/lib" % [ENV['AIS_TEST_ROOT']]
require 'AISTestUtils'
require 'test/unit'

msgDir = "%s/AIS-msg-%s" % \
         [ENV['AIS_TEST_ROOT'], 
          AISTestUtils::AISTestUtils.getAISLibVersion()]
$: << msgDir

class SelectionObjectGetNullSelectionObjectCase < Test::Unit::TestCase
    require 'MSGTestDriver'

    def test_run()
        driver = MSGTestDriver::MSGTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.init(resourceID, "SA_DISPATCH_ALL",
                    AISTestUtils::AISTestUtils.SA_AIS_OK)
        driver.selectObjectGet(resourceID, true,
                               AISTestUtils::AISTestUtils.SA_AIS_ERR_INVALID_PARAM)
        driver.stop()
    end
end
