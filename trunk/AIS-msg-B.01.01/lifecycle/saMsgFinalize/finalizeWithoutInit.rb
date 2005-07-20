#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

msgDir = "%s/AIS-msg-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << msgDir

class FinalizeWithoutInitCase < Test::Unit::TestCase
    require 'MSGTestDriver'

    def test_run()
        driver = MSGTestDriver::MSGTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.finalize(resourceID,
                        SAFTestUtils::SAFTestUtils.SA_AIS_ERR_BAD_HANDLE)
        driver.stop()
    end
end
