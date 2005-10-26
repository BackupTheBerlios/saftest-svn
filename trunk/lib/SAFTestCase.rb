module SAFTest

SA_SAF_OK=1
SA_SAF_ERR_LIBRARY=2

#if ! ENV.has_key?('SB_ROOT') then

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]

class SAFTestCase
    PASSED_EXIT_STATUS = 0
    FAILED_EXIT_STATUS = 1
    SKIPPED_EXIT_STATUS = 2
    NOT_CONFIGURED_EXIT_STATUS = 3

    def setup()
        require 'SAFSys'
        require 'SAFTestUtils'

        @utils = SAFTestUtils::SAFTestUtils.new()

        # make sure all output goes to stdout
        # we do this so sb can easily use one pipe to capture all output
        $stderr = $stdout
    end

    def runDir()
        return @utils.runDir()
    end

    def setLogLevel(level)
        @utils.setLogLevel(level)
    end

    def log(message)
        @utils.log(message)
    end

    def passed()
        print "PASSED\n"
        exit PASSED_EXIT_STATUS
    end

    def failed(message)
        print "FAILED: #{message}\n"
        exit FAILED_EXIT_STATUS
    end

    def skipped(message)
        print "SKIPPED: #{message}\n"
        exit SKIPPED_EXIT_STATUS
    end

    def notConfigured(message)
        print "NOT CONFIGURED: #{message}\n"
        exit NOT_CONFIGURED_EXIT_STATUS
    end

    def logCommand(cmd, node, user)
        @utils.logCommand(cmd, node, user)
    end

    def runCommand(cmd, node=nil, user=nil)
        @utils.logCommand(cmd, node, user)
        return @utils.runCommand(cmd, node, user)
    end

    def runAndCheckCommand(cmd, expectZeroStatus, errorMessage="", 
                           node=nil, user=nil)
        @utils.logCommand(cmd, node, user)
        @utils.runAndCheckCommand(cmd, expectZeroStatus, errorMessage, 
                                  node, user)
    end

    def captureCommand(cmd, node=nil, user=nil)
        @utils.logCommand(cmd, node, user)
        return @utils.captureCommand(cmd, node, user)
    end

end # class

end # module
