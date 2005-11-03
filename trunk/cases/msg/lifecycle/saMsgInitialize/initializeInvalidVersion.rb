#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

msgDir = "%s/AIS-msg-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << msgDir

class InitializeInvalidVersionCase < Test::Unit::TestCase
    require 'MSGTestDriver'

    def test_run()
        driver = MSGTestDriver::MSGTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.initWithOptions(resourceID, "SA_DISPATCH_ALL",
                               '0', '0', '0',
                               false, false, false,
                               SAFTestUtils::SAFTestUtils.SA_AIS_ERR_VERSION)

        driver.stop()
    end
end
