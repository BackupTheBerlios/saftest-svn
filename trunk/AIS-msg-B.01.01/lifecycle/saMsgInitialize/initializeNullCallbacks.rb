#!/usr/bin/ruby

$: << "%s/lib" % [ENV['AIS_TEST_ROOT']]
require 'AISTestUtils'
require 'test/unit'

msgDir = "%s/AIS-msg-%s" % \
         [ENV['AIS_TEST_ROOT'], 
          AISTestUtils::AISTestUtils.getAISLibVersion()]
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
            AISTestUtils::AISTestUtils.SA_AIS_RELEASE_CODE_INTEGER,
            AISTestUtils::AISTestUtils.SA_AIS_MAJOR_VERSION_INTEGER,
            AISTestUtils::AISTestUtils.SA_AIS_MINOR_VERSION_INTEGER,
            false, true, false,
            AISTestUtils::AISTestUtils.SA_AIS_ERR_INVALID_PARAM)

        driver.stop()
    end
end
