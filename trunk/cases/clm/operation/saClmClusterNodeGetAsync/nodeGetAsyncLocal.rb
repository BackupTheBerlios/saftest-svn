#!/usr/bin/ruby

module SAFTest

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestCase'
require 'SAFTestUtils'

clmDir = "%s/cases/clm" % [ENV['SAFTEST_ROOT']]
$: << clmDir
require 'CLMTestDriver'

class NodeGetAsyncLocalCase < SAFTestCase
    def initialize()
        super()
    end

    def run()
        d = CLMTestDriver.getRandomLongLivedDriver(nil)
        resource = d.getRandomTestResource()
        localNode = d.getImplementation().getCluster().getLocalNode()
        d.clusterNodeGetAsync(resource, "SA_CLM_LOCAL_NODE_ID", 
                              localNode, SAFTestUtils.SA_AIS_OK)
        passed()
    end
end

test = NodeGetAsyncLocalCase.new()
test.run()

end # module
