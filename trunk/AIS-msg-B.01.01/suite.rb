#!/usr/bin/ruby

$: << "%s/lib" % [ENV['AIS_TEST_ROOT']]
require 'AISTestUtils'

msgDir = "%s/AIS-msg-%s" % \
         [ENV['AIS_TEST_ROOT'],
          AISTestUtils::AISTestUtils.getAISLibVersion()]
$: << msgDir

require 'AISTestSuite'
require 'test/unit/testsuite'

require 'lifecycle/saMsgInitialize/initialize'
require 'lifecycle/saMsgInitialize/initializeNullCallbacks'
require 'lifecycle/saMsgInitialize/initializeNullHandle'
require 'lifecycle/saMsgInitialize/initializeInvalidVersion'
require 'lifecycle/saMsgInitialize/initializeNullVersion'

require 'lifecycle/saMsgFinalize/finalize'
require 'lifecycle/saMsgFinalize/finalizeTwice'
require 'lifecycle/saMsgFinalize/finalizeWithoutInit'

require 'lifecycle/saMsgSelectionObjectGet/selectionObjectGet'
require 'lifecycle/saMsgSelectionObjectGet/selectionObjectGetNullSelectionObject'
require 'lifecycle/saMsgSelectionObjectGet/selectionObjectGetInvalidHandle'
require 'lifecycle/saMsgSelectionObjectGet/selectionObjectGetFinalizedHandle'

#require 'lifecycle/saMsgDispatch/dispatchOne'
#require 'lifecycle/saMsgDispatch/dispatchAll'
#require 'lifecycle/saMsgDispatch/dispatchBlocking'
require 'lifecycle/saMsgDispatch/dispatchAllInvalidHandle'
require 'lifecycle/saMsgDispatch/dispatchAllInvalidFlags'
require 'lifecycle/saMsgDispatch/dispatchAllFinalizedHandle'

class MSGTestSuite < AISTestSuite::AISTestSuite
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

        #@suites << DispatchOneCase.suite()
        #@suites << DispatchAllCase.suite()
        #@suites << DispatchBlockingCase.suite()
        @suites << DispatchAllInvalidHandleCase.suite()
        @suites << DispatchAllInvalidFlagsCase.suite()
        @suites << DispatchAllFinalizedHandleCase.suite()

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

instance = MSGTestSuite.new()
