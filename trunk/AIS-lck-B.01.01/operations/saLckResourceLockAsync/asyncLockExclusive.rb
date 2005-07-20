#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

lckDir = "%s/AIS-lck-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << lckDir

class AsyncLockEXCase < Test::Unit::TestCase
    require 'LCKTestDriver'
    @@INVOCATION = $$

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
        driver.lockAsync(resourceID, 'EX', @@INVOCATION, false, false, false, 
                         false,
                         SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        sleep(2)
        driver.lockGetAsyncLockStatus(resourceID, @@INVOCATION,
                                      'SA_LCK_LOCK_GRANTED',
                                      SAFTestUtils::SAFTestUtils.SA_AIS_OK)

        driver.stop()
    end
end
