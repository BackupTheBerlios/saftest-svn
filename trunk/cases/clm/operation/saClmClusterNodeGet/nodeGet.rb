#!/usr/bin/ruby

module SAFTest

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestCase'
require 'SAFTestUtils'

clmDir = "%s/cases/clm" % [ENV['SAFTEST_ROOT']]
$: << clmDir
require 'CLMTestDriver'

class NodeGetCase < SAFTestCase
    def initialize()
        super()
    end

    def run()
        d = CLMTestDriver.getRandomLongLivedDriver(nil)
        resource = d.getRandomTestResource()
        validateNode = nil

        nodeIDParam = getParam('node-id')
        if nodeIDParam == 'LOCAL'
            validateNode = d.getImplementation().getCluster().getLocalNode()
            nodeID = "SA_CLM_LOCAL_NODE_ID"
        elsif nodeIDParam == 'RANDOM'
            validateNode = d.getImplementation().getCluster().getRandomNode()
            nodeID = validateNode.getID()
        else
            raise "Invalid nodeIDParam #{nodeIDParam}"
        end
            
        d.clusterNodeGet(resource, nodeID, "SA_TIME_MAX", 
                         false, validateNode, SAFTestUtils.SA_AIS_OK)
        passed()
    end
end

test = NodeGetCase.new()
test.run()

end # module
