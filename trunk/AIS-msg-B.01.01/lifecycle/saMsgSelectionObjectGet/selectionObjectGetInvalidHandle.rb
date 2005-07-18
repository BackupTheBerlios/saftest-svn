#!/usr/bin/ruby

$: << "%s/lib" % [ENV['AIS_TEST_ROOT']]
require 'AISTestUtils'
require 'test/unit'

msgDir = "%s/AIS-msg-%s" % \
         [ENV['AIS_TEST_ROOT'], 
          AISTestUtils::AISTestUtils.getAISLibVersion()]
$: << msgDir

class SelectionObjectGetInvalidHandleCase < Test::Unit::TestCase
    require 'MSGTestDriver'

    def test_run()
        driver = MSGTestDriver::MSGTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.selectObjectGet(resourceID, false,
                               AISTestUtils::AISTestUtils.SA_AIS_ERR_BAD_HANDLE)
        driver.stop()
    end
end
