#!/usr/bin/ruby

$: << "%s/lib" % [ENV['AIS_TEST_ROOT']]
require 'AISTestUtils'
require 'test/unit'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['AIS_TEST_ROOT'], 
          AISTestUtils::AISTestUtils.getAISLibVersion()]
$: << clmDir

class NodeGetAsyncLocalWithoutInitCase < Test::Unit::TestCase
    require 'CLMTestDriver'

    def test_run()
        driver = CLMTestDriver::CLMTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        randomInvocation = driver.generateInvocation()
        driver.clusterNodeGetAsync(resourceID, randomInvocation, 
                                   "SA_CLM_LOCAL_NODE_ID",
                                   AISTestUtils::AISTestUtils.SA_AIS_ERR_BAD_HANDLE)
        driver.stop()
    end
end
