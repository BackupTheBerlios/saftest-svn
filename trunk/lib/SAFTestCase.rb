module SAFTestCase

SA_SAF_OK=1
SA_SAF_ERR_LIBRARY=2

#if ! ENV.has_key?('SB_ROOT') then

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]

require 'test/unit'

class SAFTestCase < Test::Unit::TestCase
    def setup()
        require 'SAFSys'
        require 'SAFTestUtils'

        @utils = SAFTestUtils::SAFTestUtils.new()

        # make sure all output goes to stdout
        # we do this so sb can easily use one pipe to capture all output
        $stderr = $stdout
    end

    def getRunDir()
        return @utils.getRunDir()
    end

    def setLogLevel(level)
        @utils.setLogLevel(level)
    end

    def log(message)
        @utils.log(message)
    end

    def passed()
        @utils.passed()
    end

    def failed(message)
        @utils.failed(message)
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
