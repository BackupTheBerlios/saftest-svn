#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << clmDir

class InitializeNullHandleCase < Test::Unit::TestCase
    require 'CLMTestDriver'

    def test_run()
        driver = CLMTestDriver::CLMTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.initWithOptions(
            resourceID, "SA_DISPATCH_ALL",
            SAFTestUtils::SAFTestUtils.SA_AIS_RELEASE_CODE,
            SAFTestUtils::SAFTestUtils.SA_AIS_MAJOR_VERSION,
            SAFTestUtils::SAFTestUtils.SA_AIS_MINOR_VERSION,
            false, false, true,
            SAFTestUtils::SAFTestUtils.SA_AIS_ERR_VERSION)

        driver.stop()
    end
end
