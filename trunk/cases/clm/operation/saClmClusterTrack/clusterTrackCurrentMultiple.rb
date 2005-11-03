#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << clmDir

class ClusterTrackCurrentMultipleCase < Test::Unit::TestCase
    require 'CLMTestDriver'
    @@TIMES_TO_RUN = 5

    def test_run()
        driver = CLMTestDriver::CLMTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.init(resourceID, true, true, "SA_DISPATCH_ALL",
                    SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.selectObjectGet(resourceID, false,
                               SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        @@TIMES_TO_RUN.times do 
            driver.clusterTrack(resourceID, true, false, false, false, 
                                true, false, 0,
                                SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        end
        driver.dispatch(resourceID, "SA_DISPATCH_ALL", 
                        SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.clusterTrackCBCount(resourceID, @@TIMES_TO_RUN)
        driver.finalize(resourceID, SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.stop()
    end
end
