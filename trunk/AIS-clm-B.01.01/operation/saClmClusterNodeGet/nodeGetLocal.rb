#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << clmDir

class NodeGetLocalCase < Test::Unit::TestCase
    require 'CLMTestDriver'

    def test_run()
        CLMTestDriver::CLMTestDriver.getLongLivedDrivers(nil).each do |d|
            resourceID = d.getRandomTestResourceID()
            d.clusterNodeGet(resourceID, "SA_CLM_LOCAL_NODE_ID",
                             100, false, 
                             SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        end
    end
end
