#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

lckDir = "%s/AIS-lck-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << lckDir

class FinalizeTwiceCase < Test::Unit::TestCase
    require 'LCKTestDriver'

    def test_run()
        driver = LCKTestDriver::LCKTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.init(resourceID, "SA_DISPATCH_ONE",
                     SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.finalize(resourceID,
                        SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.finalize(resourceID,
                        SAFTestUtils::SAFTestUtils.SA_AIS_ERR_BAD_HANDLE)
        driver.stop()
    end
end
