#!/usr/bin/ruby

module SAFTest

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestCase'
require 'SAFTestUtils'

clmDir = "%s/cases/clm" % [ENV['SAFTEST_ROOT']]
$: << clmDir
require 'CLMTestDriver'

class NodeGetLocalCase < SAFTestCase
    def initialize()
        super()
    end

    def run()
        d = CLMTestDriver.getRandomLongLivedDriver(nil)
        resource = d.getRandomTestResource()
        localNode = d.getImplementation().getCluster().getLocalNode()
        d.clusterNodeGet(resource, "SA_CLM_LOCAL_NODE_ID", "SA_TIME_MAX", 
                         false, localNode, SAFTestUtils.SA_AIS_OK)
        passed()
    end
end

test = NodeGetLocalCase.new()
test.run()

end # module
