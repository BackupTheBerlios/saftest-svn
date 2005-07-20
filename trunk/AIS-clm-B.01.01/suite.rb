#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['SAFTEST_ROOT'],
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << clmDir

require 'SAFTestSuite'
require 'test/unit/testsuite'

require 'lifecycle/saClmInitialize/initialize'
require 'lifecycle/saClmInitialize/initializeNullCallbacks'
require 'lifecycle/saClmInitialize/initializeNullHandle'
require 'lifecycle/saClmInitialize/initializeInvalidVersion'
require 'lifecycle/saClmInitialize/initializeNullVersion'

require 'lifecycle/saClmFinalize/finalize'
require 'lifecycle/saClmFinalize/finalizeTwice'
require 'lifecycle/saClmFinalize/finalizeWithoutInit'

require 'lifecycle/saClmSelectionObjectGet/selectionObjectGet'
require 'lifecycle/saClmSelectionObjectGet/selectionObjectGetNullSelectionObject'
require 'lifecycle/saClmSelectionObjectGet/selectionObjectGetInvalidHandle'
require 'lifecycle/saClmSelectionObjectGet/selectionObjectGetFinalizedHandle'

require 'lifecycle/saClmDispatch/dispatchOne'
require 'lifecycle/saClmDispatch/dispatchAll'
require 'lifecycle/saClmDispatch/dispatchBlocking'
require 'lifecycle/saClmDispatch/dispatchAllInvalidHandle'
require 'lifecycle/saClmDispatch/dispatchAllInvalidFlags'
require 'lifecycle/saClmDispatch/dispatchAllFinalizedHandle'

require 'operation/saClmClusterNodeGet/nodeGetLocalFinalizedHandle'
require 'operation/saClmClusterNodeGet/nodeGetLocalWithoutInit'
require 'operation/saClmClusterNodeGet/nodeGetLocalNullClusterNode'
require 'operation/saClmClusterNodeGet/nodeGetLocalZeroTimeout'
require 'operation/saClmClusterNodeGet/nodeGetLocal'

require 'operation/saClmClusterNodeGetAsync/nodeGetAsyncLocalFinalizedHandle'
require 'operation/saClmClusterNodeGetAsync/nodeGetAsyncLocal'
require 'operation/saClmClusterNodeGetAsync/nodeGetAsyncLocalNoCallback'
require 'operation/saClmClusterNodeGetAsync/nodeGetAsyncLocalWithoutInit'

require 'operation/saClmClusterTrack/clusterTrackChangesAndChangesOnly'
require 'operation/saClmClusterTrack/clusterTrackChangesWithNodeRunAndStop'
require 'operation/saClmClusterTrack/clusterTrackCurrentFinalizedHandle'
require 'operation/saClmClusterTrack/clusterTrackCurrentMultiple'
require 'operation/saClmClusterTrack/clusterTrackCurrentNoCallback'
require 'operation/saClmClusterTrack/clusterTrackCurrentNullNotificationBuffer'
require 'operation/saClmClusterTrack/clusterTrackCurrent'
require 'operation/saClmClusterTrack/clusterTrackCurrentWithoutInit'
require 'operation/saClmClusterTrack/clusterTrackInvalidFlags'
require 'operation/saClmClusterTrack/clusterTrackInvalidNumberOfItems'

require 'operation/saClmClusterTrackStop/clusterTrackChangesStopTwice'
require 'operation/saClmClusterTrackStop/clusterTrackChangesOnlyStopTwice'
require 'operation/saClmClusterTrackStop/clusterTrackChangesStop'
require 'operation/saClmClusterTrackStop/clusterTrackStopWithoutTrack'
require 'operation/saClmClusterTrackStop/clusterTrackStopFinalizedHandle'
require 'operation/saClmClusterTrackStop/clusterTrackStopInvalidHandle'
require 'operation/saClmClusterTrackStop/clusterTrackStopWithTrackCurrent'

class CLMTestSuite < SAFTestSuite::SAFTestSuite
    def initialize()
        @suites = []

        @suites << InitializeInvalidVersionCase.suite()
        @suites << InitializeNullCallbacksCase.suite()
        @suites << InitializeNullHandleCase.suite()
        @suites << InitializeNullHandleCase.suite()
        @suites << InitializeCase.suite()

        @suites << FinalizeCase.suite()
        @suites << FinalizeTwiceCase.suite()
        @suites << FinalizeWithoutInitCase.suite()

        @suites << SelectionObjectGetCase.suite()
        @suites << SelectionObjectGetNullSelectionObjectCase.suite()
        @suites << SelectionObjectGetInvalidHandleCase.suite()
        @suites << SelectionObjectGetFinalizedHandleCase.suite()

        @suites << DispatchOneCase.suite()
        @suites << DispatchAllCase.suite()
        @suites << DispatchBlockingCase.suite()
        @suites << DispatchAllInvalidHandleCase.suite()
        @suites << DispatchAllInvalidFlagsCase.suite()
        @suites << DispatchAllFinalizedHandleCase.suite()

        @suites << NodeGetLocalFinalizedHandleCase.suite()
        @suites << NodeGetLocalWithoutInitCase.suite()
        @suites << NodeGetLocalNullClusterNodeCase.suite()
        @suites << NodeGetLocalZeroTimeoutCase.suite()
        @suites << NodeGetLocalCase.suite()

        @suites << NodeGetAsyncLocalFinalizedHandleCase.suite()
        @suites << NodeGetAsyncLocalCase.suite()
        @suites << NodeGetAsyncLocalNoCallbackCase.suite()
        @suites << NodeGetAsyncLocalWithoutInitCase.suite()

        @suites << ClusterTrackChangesAndChangesOnlyCase.suite()
        @suites << ClusterTrackChangesWithNodeRunAndStopCase.suite()
        @suites << ClusterTrackCurrentFinalizedHandleCase.suite()
        @suites << ClusterTrackCurrentMultipleCase.suite()
        @suites << ClusterTrackCurrentNoCallbackCase.suite()
        @suites << ClusterTrackCurrentNullNotificationBufferCase.suite()
        @suites << ClusterTrackCurrentCase.suite()
        @suites << ClusterTrackCurrentWithoutInitCase.suite()
        @suites << ClusterTrackInvalidFlagsCase.suite()
        @suites << ClusterTrackInvalidNumberOfItemsCase.suite()

        @suites << ClusterTrackChangesStopTwiceCase.suite()
        @suites << ClusterTrackChangesOnlyStopTwiceCase.suite()
        @suites << ClusterTrackChangesStopCase.suite()
        @suites << ClusterTrackStopWithoutTrackCase.suite()
        @suites << ClusterTrackStopFinalizedHandleCase.suite()
        @suites << ClusterTrackStopInvalidHandleCase.suite()
        @suites << ClusterTrackStopWithTrackCurrentCase.suite()
    end

    def size()
        return @suites.length
    end

    def suite()
        suite = Test::Unit::TestSuite.new()
        @suites.each { |s|
            suite << s
        }
        return suite
    end

end # class

instance = CLMTestSuite.new()
