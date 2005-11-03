#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

lckDir = "%s/AIS-lck-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << lckDir

class UnlockEXAsyncLockCase < Test::Unit::TestCase
    require 'SAFSys'
    require 'LCKTestDriver'

    def test_run()
        #driver = LCKTestDriver::LCKTestDriver.new()
        #driver.start()
        #resourceID = driver.createTestResource()
        #driver.init(resourceID, "SA_DISPATCH_ALL",
                    #SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        #driver.selectObjectGet(resourceID, 
                               #SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        #driver.resourceOpen(resourceID, "lock1", 
                            #SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        #driver.lockSync(resourceID, 'EX', false, false, true, false, false,
                        #'SA_LCK_LOCK_GRANTED', 
                        #SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        #driver.unlockSync(resourceID, 
                          #SAFTestUtils::SAFTestUtils.SA_AIS_OK)

        #driver.stop()
        raise "need to implement"
    end
end
