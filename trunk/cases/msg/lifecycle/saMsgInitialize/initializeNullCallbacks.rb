#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

msgDir = "%s/AIS-msg-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << msgDir

class InitializeNullCallbacksCase < Test::Unit::TestCase
    require 'MSGTestDriver'

    def test_run()
        driver = MSGTestDriver::MSGTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.initWithOptions(
            resourceID, "SA_DISPATCH_ALL",
            SAFTestUtils::SAFTestUtils.SA_AIS_RELEASE_CODE,
            SAFTestUtils::SAFTestUtils.SA_AIS_MAJOR_VERSION,
            SAFTestUtils::SAFTestUtils.SA_AIS_MINOR_VERSION,
            false, true, false,
            SAFTestUtils::SAFTestUtils.SA_AIS_ERR_INVALID_PARAM)

        driver.stop()
    end
end
