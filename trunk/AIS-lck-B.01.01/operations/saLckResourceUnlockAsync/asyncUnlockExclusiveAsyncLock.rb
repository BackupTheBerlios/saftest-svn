#!/usr/bin/ruby

$: << "%s/lib" % [ENV['AIS_TEST_ROOT']]
require 'AISTestUtils'
require 'test/unit'

lckDir = "%s/AIS-lck-%s" % \
         [ENV['AIS_TEST_ROOT'], 
          AISTestUtils::AISTestUtils.getAISLibVersion()]
$: << lckDir

class AsyncUnlockEXAsyncLockCase < Test::Unit::TestCase
    require 'LCKTestDriver'
    @@LOCK_INVOCATION = $$ 
    @@UNLOCK_INVOCATION = $$ + 1

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
        driver.lockAsync(resourceID, 'EX', @@LOCK_INVOCATION, 
                         false, false, false, false,
                         AISTestUtils::AISTestUtils.SA_AIS_OK)
        sleep(2)
        driver.lockGetAsyncLockStatus(resourceID, @@LOCK_INVOCATION,
                                      'SA_LCK_LOCK_GRANTED',
                                      AISTestUtils::AISTestUtils.SA_AIS_OK)
        driver.unlockAsync(resourceID, @@UNLOCK_INVOCATION,
                           AISTestUtils::AISTestUtils.SA_AIS_OK)
        sleep(2)
        driver.lockGetAsyncUnlockStatus(resourceID, @@UNLOCK_INVOCATION,
                                        'SA_LCK_LOCK_NO_MORE',
                                        AISTestUtils::AISTestUtils.SA_AIS_OK)

        driver.stop()
    end
end
