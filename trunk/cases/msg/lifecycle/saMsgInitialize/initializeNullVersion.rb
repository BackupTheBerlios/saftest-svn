#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

msgDir = "%s/AIS-msg-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << msgDir

class InitializeNullHandleCase < Test::Unit::TestCase
    require 'MSGTestDriver'

    def test_run()
        driver = MSGTestDriver::MSGTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.initWithOptions(
            resourceID, "SA_DISPATCH_ALL",
            SAFTestUtils::SAFTestUtils.SA_AIS_RELEASE_CODE_INTEGER,
            SAFTestUtils::SAFTestUtils.SA_AIS_MAJOR_VERSION_INTEGER,
            SAFTestUtils::SAFTestUtils.SA_AIS_MINOR_VERSION_INTEGER,
            false, false, true,
            SAFTestUtils::SAFTestUtils.SA_AIS_ERR_VERSION)

        driver.stop()
    end
end
