#!/usr/bin/ruby

$: << "%s/lib" % [ENV['AIS_TEST_ROOT']]
require 'AISTestUtils'
require 'test/unit'

lckDir = "%s/AIS-lck-%s" % \
         [ENV['AIS_TEST_ROOT'], 
          AISTestUtils::AISTestUtils.getAISLibVersion()]
$: << lckDir

class UnlockEXSyncLockCase < Test::Unit::TestCase
    require 'AISSys'
    require 'LCKTestDriver'

    def test_run()
        driver = LCKTestDriver::LCKTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.init(resourceID, "SA_DISPATCH_ALL",
                    AISTestUtils::AISTestUtils.SA_AIS_OK)
        driver.selectObjectGet(resourceID, false,
                               AISTestUtils::AISTestUtils.SA_AIS_OK)
        driver.resourceOpen(resourceID, "lock1", 
                            AISTestUtils::AISTestUtils.SA_AIS_OK)
        driver.lockSync(resourceID, 'EX', 0, false, false, true, false, false,
                        'SA_LCK_LOCK_GRANTED', 
                        AISTestUtils::AISTestUtils.SA_AIS_OK)
        driver.unlockSync(resourceID, 
                          AISTestUtils::AISTestUtils.SA_AIS_OK)

        driver.stop()
    end
end
