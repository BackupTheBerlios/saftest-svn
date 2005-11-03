#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'SAFTestCase'
require 'test/unit'

lckDir = "%s/AIS-lck-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << lckDir

class InitializeNullHandleCase < Test::Unit::TestCase
    require 'LCKTestDriver'

    def test_run()
        driver = LCKTestDriver::LCKTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.initWithOptions(
            resourceID, "SA_DISPATCH_ALL",
            SAFTestUtils::SAFTestUtils.SA_AIS_RELEASE_CODE_INTEGER,
            SAFTestUtils::SAFTestUtils.SA_AIS_MAJOR_VERSION_INTEGER,
            SAFTestUtils::SAFTestUtils.SA_AIS_MINOR_VERSION_INTEGER,
            true, false, false,
            SAFTestUtils::SAFTestUtils.SA_AIS_ERR_INVALID_PARAM)

        driver.stop()
    end
end

