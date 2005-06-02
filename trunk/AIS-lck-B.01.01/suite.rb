#!/usr/bin/ruby

$: << "%s/lib" % [ENV['AIS_TEST_ROOT']]
require 'AISTestUtils'

lckDir = "%s/AIS-lck-%s" % \
         [ENV['AIS_TEST_ROOT'],
          AISTestUtils::AISTestUtils.getAISLibVersion()]
$: << lckDir

require 'AISTestSuite'
require 'test/unit/testsuite'

require 'lifecycle/saLckInitialize/initialize'
require 'lifecycle/saLckInitialize/initializeNullCallbacks'
require 'lifecycle/saLckInitialize/initializeNullHandle'
require 'lifecycle/saLckInitialize/initializeInvalidVersion'
require 'lifecycle/saLckInitialize/initializeNullVersion'

require 'lifecycle/saLckFinalize/finalize'
require 'lifecycle/saLckFinalize/finalizeTwice'
require 'lifecycle/saLckFinalize/finalizeWithoutInit'

require 'lifecycle/saLckSelectionObjectGet/selectionObjectGet'
require 'lifecycle/saLckSelectionObjectGet/selectionObjectGetNullSelectionObject'
require 'lifecycle/saLckSelectionObjectGet/selectionObjectGetInvalidHandle'
require 'lifecycle/saLckSelectionObjectGet/selectionObjectGetFinalizedHandle'

require 'lifecycle/saLckDispatch/dispatchOne'
require 'lifecycle/saLckDispatch/dispatchAll'
require 'lifecycle/saLckDispatch/dispatchBlocking'
require 'lifecycle/saLckDispatch/dispatchAllInvalidHandle'
require 'lifecycle/saLckDispatch/dispatchAllInvalidFlags'
require 'lifecycle/saLckDispatch/dispatchAllFinalizedHandle'

require 'operations/saLckResourceClose/resourceClose'
require 'operations/saLckResourceClose/resourceCloseTwice'
require 'operations/saLckResourceClose/resourceCloseFinalizedHandle'
require 'operations/saLckResourceClose/resourceCloseUnopenedResource'

require 'operations/saLckResourceLock/lockExclusive'
require 'operations/saLckResourceLock/lockProtectedRead'
require 'operations/saLckResourceLock/lockInvalidFlag'
require 'operations/saLckResourceLock/lockInvalidMode'
require 'operations/saLckResourceLock/lockExclusiveNullStatus'
require 'operations/saLckResourceLock/lockExclusiveNullLockID'
require 'operations/saLckResourceLock/lockExclusiveNoResourceOpen'
require 'operations/saLckResourceLock/lockExclusiveClosedResource'

require 'operations/saLckResourceUnlock/unlockExclusiveFinalizedHandle'
require 'operations/saLckResourceUnlock/unlockProtectedReadSyncLock'
require 'operations/saLckResourceUnlock/unlockExclusiveSyncLock'
require 'operations/saLckResourceUnlock/unlockInvalidLockID'
require 'operations/saLckResourceUnlock/unlockExclusiveAsyncLock'
require 'operations/saLckResourceUnlock/unlockProtectedReadAsyncLock'

class LCKTestSuite < AISTestSuite::AISTestSuite
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

        @suites << ResourceCloseCase.suite()
        @suites << ResourceCloseTwiceCase.suite()
        @suites << ResourceCloseFinalizedHandleCase.suite()
        @suites << ResourceCloseUnopenedResourceCase.suite()

        @suites << LockEXCase.suite()
        @suites << LockPRCase.suite()
        @suites << LockInvalidFlagCase.suite()
        @suites << LockInvalidModeCase.suite()
        @suites << LockEXNullStatusCase.suite()
        @suites << LockEXNullLockIDCase.suite()
        @suites << LockEXNoResourceOpenCase.suite()
        @suites << LockEXClosedResourceCase.suite()

        @suites << UnlockEXFinalizedHandleCase.suite()
        @suites << UnlockPRSyncLockCase.suite()
        @suites << UnlockEXSyncLockCase.suite()
        @suites << UnlockInvalidLockIDCase.suite()
        #@suites << UnlockEXAsyncLockCase.suite()
        #@suites << UnlockPRAsyncLockCase.suite()
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

instance = LCKTestSuite.new()
