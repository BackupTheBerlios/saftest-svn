#!/usr/bin/ruby

$: << "%s/lib" % [ENV['AIS_TEST_ROOT']]
require 'AISTestUtils'
require 'test/unit'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['AIS_TEST_ROOT'], 
          AISTestUtils::AISTestUtils.getAISLibVersion()]
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
                    AISTestUtils::AISTestUtils.SA_AIS_OK)
        driver.selectObjectGet(resourceID, false,
                               AISTestUtils::AISTestUtils.SA_AIS_OK)
        @@TIMES_TO_RUN.times do 
            driver.clusterTrack(resourceID, true, false, false, false, 
                                true, false, 0,
                                AISTestUtils::AISTestUtils.SA_AIS_OK)
        end
        driver.dispatch(resourceID, "SA_DISPATCH_ALL", 
                        AISTestUtils::AISTestUtils.SA_AIS_OK)
        driver.clusterTrackCBCount(resourceID, @@TIMES_TO_RUN)
        driver.finalize(resourceID, AISTestUtils::AISTestUtils.SA_AIS_OK)
        driver.stop()
    end
end
