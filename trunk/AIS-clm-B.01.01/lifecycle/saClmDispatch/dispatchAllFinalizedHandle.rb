#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << clmDir

class DispatchAllFinalizedHandleCase < Test::Unit::TestCase
    require 'CLMTestDriver'

    def test_run()
        driver = CLMTestDriver::CLMTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.init(resourceID, false, false, "SA_DISPATCH_ALL",
                    SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.finalize(resourceID, SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.dispatch(resourceID, "SA_DISPATCH_ALL", 
                        SAFTestUtils::SAFTestUtils.SA_AIS_ERR_BAD_HANDLE)

        driver.stop()
    end
end
