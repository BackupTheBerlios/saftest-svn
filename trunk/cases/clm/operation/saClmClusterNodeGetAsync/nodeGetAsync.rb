#!/usr/bin/ruby

module SAFTest

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestCase'
require 'SAFTestUtils'

clmDir = "%s/cases/clm" % [ENV['SAFTEST_ROOT']]
$: << clmDir
require 'CLMTestDriver'

class NodeGetAsyncCase < SAFTestCase
    def initialize()
        super()
    end

    def run()
        d = CLMTestDriver.getRandomLongLivedDriver(nil)
        resource = d.getRandomTestResource()
        validateNode = nil

        nodeIDParam = getParam('node-id')
        if nodeIDParam == 'local'
            validateNode = d.getImplementation().getCluster().getLocalNode()
            nodeID = "SA_CLM_LOCAL_NODE_ID"
        elsif nodeIDParam == 'random'
            validateNode = d.getImplementation().getCluster().getRandomNode()
            nodeID = validateNode.getID()
        else
            raise "Invalid nodeIDParam #{nodeIDParam}"
        end

        d.clusterNodeGetAsync(resource, nodeID, validateNode,
                              SAFTestUtils.SA_AIS_OK)
        passed()
    end
end

test = NodeGetAsyncCase.new()
test.run()

end # module
