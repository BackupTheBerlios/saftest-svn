#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

lckDir = "%s/AIS-lck-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << lckDir

class DispatchAllCase < Test::Unit::TestCase
    require 'LCKTestDriver'
    @@WAITER_SIGNAL = $$

    def test_run()
        driver1 = LCKTestDriver::LCKTestDriver.new(nil)
        driver1.killAllDrivers()
        driver1.start()
        resource1ID = driver1.createTestResource()
        driver1.init(resource1ID, "SA_DISPATCH_ALL",
                     SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver1.selectObjectGet(resource1ID, false,
                               SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver1.resourceOpen(resource1ID, "lock1", 
                            SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver1.lockSync(resource1ID, 'EX', 0, false, false, true, false, false,
                         'SA_LCK_LOCK_GRANTED',
                        SAFTestUtils::SAFTestUtils.SA_AIS_OK)

        driver1.lockGetWaitCount(resource1ID, 0, 0)

        driver2 = LCKTestDriver::LCKTestDriver.new(nil)
        driver2.start()
        resource2ID = driver2.createTestResource()
        driver2.init(resource2ID, "SA_DISPATCH_ALL",
                     SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver2.selectObjectGet(resource2ID, false,
                               SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver2.resourceOpen(resource2ID, "lock1", 
                            SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver2.lockSync(resource2ID, 'EX', @@WAITER_SIGNAL,
                         false, false, true, false, false,
                         'SA_LCK_LOCK_NOT_QUEUED',
                         SAFTestUtils::SAFTestUtils.SA_AIS_OK)

        driver1.lockGetWaitCount(resource1ID, @@WAITER_SIGNAL, 1)

        driver1.stop()
        driver2.stop()
    end
end
