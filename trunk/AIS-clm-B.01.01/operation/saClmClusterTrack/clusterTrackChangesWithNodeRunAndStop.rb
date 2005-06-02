#!/usr/bin/ruby

$: << "%s/lib" % [ENV['AIS_TEST_ROOT']]
require 'AISTestUtils'
require 'AISImplementation'
require 'test/unit'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['AIS_TEST_ROOT'], 
          AISTestUtils::AISTestUtils.getAISLibVersion()]
$: << clmDir

class ClusterTrackChangesWithNodeRunAndStopCase < Test::Unit::TestCase
    require 'CLMTestDriver'

    def test_run()
        @currentCluster = nil

        @driver = CLMTestDriver::CLMTestDriver.new(nil)
        @driver.killAllDrivers()
        @driver.start()
        @resourceID = @driver.createTestResource()
        @driver.init(@resourceID, true, true, "SA_DISPATCH_ALL",
                     AISTestUtils::AISTestUtils.SA_AIS_OK)
        @driver.selectObjectGet(@resourceID, false,
                                AISTestUtils::AISTestUtils.SA_AIS_OK)
        @driver.clusterTrack(@resourceID, true, true, false, false,
                             true, true, 0,
                             AISTestUtils::AISTestUtils.SA_AIS_OK)
        @impl = @driver.getImplementation()
        tmpDir = '%s/results/tmp' % [ENV['AIS_TEST_ROOT']]
        @driver.runCommand("mkdir -p %s" % [tmpDir])
        @xmlFile = '%s/cluster.xml' % [tmpDir]
        validateCluster()

        if @currentCluster.getNodes().length < 2
            @driver.log("Need at least 2 nodes to continue, only have %d" % \
                        [@currentCluster.getNodes().length])
        else
            downNodes = @currentCluster.getDownNodes()
            if downNodes.length() > 0
                downNode = downNodes[0]
                runNodeStartScenario(downNode.getName())
                runNodeStopScenario(downNode.getName())
            else
                upNode = nil
                @currentCluster.getNodes().each do |node|
                    if !node.isLocalNode()
                        upNode = node
                        break
                    end
                end

                runNodeStopScenario(upNode.getName())
                runNodeStartScenario(upNode.getName())
            end
        end

        @driver.finalize(@resourceID, AISTestUtils::AISTestUtils.SA_AIS_OK)
        @driver.stop()
    end

    def runNodeStopScenario(nodeName)
        if !@currentCluster.getNodeByName(nodeName).getMember()
            raise "%s is not a running node" % [nodeName]
        end
        @impl.stopNode(nodeName)
        sleep 5
        validateCluster()
    end

    def runNodeStartScenario(nodeName)
        if @currentCluster.getNodeByName(nodeName).getMember()
            raise "%s is not a down node" % [nodeName]
        end
        @impl.startNode(nodeName)
        sleep 5
        validateCluster()
    end

    def validateCluster()
        @driver.dispatch(@resourceID, "SA_DISPATCH_ALL",
                         AISTestUtils::AISTestUtils.SA_AIS_OK)
        @driver.displayLastNotificationBuffer(@resourceID, @xmlFile, 
                                              AISTestUtils::AISTestUtils.SA_AIS_OK)
        cluster1 = @impl.getClusterFromCommand()
        cluster2 = @impl.getClusterFromFile(@xmlFile)
        @driver.runCommand("rm -f %s" % [@xmlFile])

        if cluster1 != cluster2
            @driver.log("Cluster 1 != Cluster 2...")
            @driver.log("Cluster 1 (from command):")
            print "Cluster 1 (From Command): \n"
            cluster1.display()

            print "\n\nCluster 2 (From File): \n"
            cluster2.display()

            raise "Cluster 1 != Cluster 2..."
        end
        @currentCluster = cluster1
    end
end
