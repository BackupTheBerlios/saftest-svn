#!/usr/bin/ruby

$: << "%s/lib" % [ENV['AIS_TEST_ROOT']]
require 'AISTestUtils'
require 'test/unit'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['AIS_TEST_ROOT'], 
          AISTestUtils::AISTestUtils.getAISLibVersion()]
$: << clmDir

class SelectionObjectGetNullSelectionObjectCase < Test::Unit::TestCase
    require 'CLMTestDriver'

    def test_run()
        driver = CLMTestDriver::CLMTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.init(resourceID, true, true, "SA_DISPATCH_ALL",
                    AISTestUtils::AISTestUtils.SA_AIS_OK)
        driver.selectObjectGet(resourceID, true,
                               AISTestUtils::AISTestUtils.SA_AIS_ERR_INVALID_PARAM)
        driver.stop()
    end
end
