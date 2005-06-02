#!/usr/bin/ruby

$: << "%s/lib" % [ENV['AIS_TEST_ROOT']]
require 'AISTestUtils'
require 'test/unit'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['AIS_TEST_ROOT'], 
          AISTestUtils::AISTestUtils.getAISLibVersion()]
$: << clmDir

class InitializeInvalidVersionCase < Test::Unit::TestCase
    require 'CLMTestDriver'

    def test_run()
        driver = CLMTestDriver::CLMTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.initWithOptions(resourceID, "SA_DISPATCH_ALL",
                               0, 0, 0,
                               false, false, false,
                               AISTestUtils::AISTestUtils.SA_AIS_ERR_VERSION)

        driver.stop()
    end
end
