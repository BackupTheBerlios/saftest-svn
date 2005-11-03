#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'SAFImplementation'
require 'test/unit'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << clmDir

class ClusterTrackCurrentCase < Test::Unit::TestCase
    require 'CLMTestDriver'

    def test_run()
        driver = CLMTestDriver::CLMTestDriver.new(nil)
        driver.killAllDrivers()
        driver.start()
        resourceID = driver.createTestResource()
        driver.init(resourceID, true, true, "SA_DISPATCH_ONE",
                    SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.selectObjectGet(resourceID, false,
                               SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.clusterTrack(resourceID, true, false, false, false,
                            true, true, 0,
                            SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        impl = driver.getImplementation()
        tmpDir = '%s/results/tmp' % [ENV['SAFTEST_ROOT']]
        driver.runCommand("mkdir -p %s" % [tmpDir])
        xmlFile = '%s/cluster.xml' % [tmpDir]
        driver.displayLastNotificationBuffer(resourceID, xmlFile, 
                                             SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        impl = driver.getImplementation()
        cluster1 = impl.getClusterFromCommand()
        cluster2 = impl.getClusterFromFile(xmlFile)
        #driver.runCommand("rm -f %s" % [xmlFile])

        if cluster1 != cluster2
            driver.log("Cluster 1 != Cluster 2...")
            driver.log("Cluster 1 (from command):")
            print "Cluster 1 (From Command): \n"
            cluster1.display()

            print "\n\nCluster 2 (From File): \n"
            cluster2.display()

            raise "Cluster 1 != Cluster 2..."
        end
                                             

        driver.finalize(resourceID, SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        driver.stop()
    end
end
