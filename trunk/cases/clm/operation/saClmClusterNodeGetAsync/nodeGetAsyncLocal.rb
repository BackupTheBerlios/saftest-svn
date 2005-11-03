#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << clmDir

class NodeGetAsyncLocalCase < Test::Unit::TestCase
    require 'CLMTestDriver'

    def test_run()
        CLMTestDriver::CLMTestDriver.getLongLivedDrivers(nil).each do |d|
            randomInvocation = d.generateInvocation()
            resourceID = d.getRandomTestResourceID()
            oldCBCount = d.clusterNodeGetCBCount(resourceID)
            d.clusterNodeGetAsync(resourceID, randomInvocation, 
                                       "SA_CLM_LOCAL_NODE_ID",
                                       SAFTestUtils::SAFTestUtils.SA_AIS_OK)
            newCBCount = d.clusterNodeGetCBCount(resourceID)
            if newCBCount != oldCBCount + 1
                raise 'currentCBCount = %d, newCBCount = %d' % [oldCBCount,
                                                                newCBCount]
            end
            d.clusterNodeGetAsyncInvocation(resourceID, randomInvocation)
        end
    end
end
