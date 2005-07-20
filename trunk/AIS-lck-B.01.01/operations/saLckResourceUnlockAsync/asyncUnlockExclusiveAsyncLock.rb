#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

lckDir = "%s/AIS-lck-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
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
                    SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.selectObjectGet(resourceID, false,
                               SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.resourceOpen(resourceID, "lock1", 
                            SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.lockAsync(resourceID, 'EX', @@LOCK_INVOCATION, 
                         false, false, false, false,
                         SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        sleep(2)
        driver.lockGetAsyncLockStatus(resourceID, @@LOCK_INVOCATION,
                                      'SA_LCK_LOCK_GRANTED',
                                      SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.unlockAsync(resourceID, @@UNLOCK_INVOCATION,
                           SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        sleep(2)
        driver.lockGetAsyncUnlockStatus(resourceID, @@UNLOCK_INVOCATION,
                                        'SA_LCK_LOCK_NO_MORE',
                                        SAFTestUtils::SAFTestUtils.SA_AIS_OK)

        driver.stop()
    end
end
