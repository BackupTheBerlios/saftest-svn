#!/usr/bin/ruby

$: << "%s/lib" % [ENV['AIS_TEST_ROOT']]
require 'AISTestUtils'
require 'test/unit'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['AIS_TEST_ROOT'], 
          AISTestUtils::AISTestUtils.getAISLibVersion()]
$: << clmDir

class ClusterTrackStopWithoutTrackCase < Test::Unit::TestCase
    require 'CLMTestDriver'

    def test_run()
        driver = CLMTestDriver::CLMTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.init(resourceID, true, true, "SA_DISPATCH_ONE",
                    AISTestUtils::AISTestUtils.SA_AIS_OK)
        driver.selectObjectGet(resourceID, false,
                               AISTestUtils::AISTestUtils.SA_AIS_OK)
        #driver.clusterTrack(resourceID, false, true, false, false,
                            #true, true, 0, 
                            #AISTestUtils::AISTestUtils.SA_AIS_OK)
        driver.clusterTrackStop(resourceID,
                                AISTestUtils::AISTestUtils.SA_AIS_ERR_NOT_EXIST)
        driver.finalize(resourceID, AISTestUtils::AISTestUtils.SA_AIS_OK)
        driver.stop()
    end
end
