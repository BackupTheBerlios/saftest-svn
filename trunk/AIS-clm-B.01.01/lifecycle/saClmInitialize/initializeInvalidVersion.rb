#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
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
                               SAFTestUtils::SAFTestUtils.SA_AIS_ERR_VERSION)

        driver.stop()
    end
end
