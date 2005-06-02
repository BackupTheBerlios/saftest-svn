#!/usr/bin/ruby

$: << "%s/lib" % [ENV['AIS_TEST_ROOT']]
require 'AISTestUtils'
require 'test/unit'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['AIS_TEST_ROOT'], 
          AISTestUtils::AISTestUtils.getAISLibVersion()]
$: << clmDir

class DispatchBlockingCase < Test::Unit::TestCase
    require 'CLMTestDriver'

    def test_run()
        driver = CLMTestDriver::CLMTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.init(resourceID, true, false, "SA_DISPATCH_BLOCKING",
                    AISTestUtils::AISTestUtils.SA_AIS_OK)
        driver.selectObjectGet(resourceID, false,
                               AISTestUtils::AISTestUtils.SA_AIS_OK)
        driver.clusterNodeGetCBCount(resourceID, 0)
        driver.clusterNodeGetAsync(resourceID, 1, "SA_CLM_LOCAL_NODE_ID", 
                         AISTestUtils::AISTestUtils.SA_AIS_OK)
        driver.clusterNodeGetCBCount(resourceID, 1)
        driver.stop()
    end
end
