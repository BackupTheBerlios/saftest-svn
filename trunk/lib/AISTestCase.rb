module AISTestCase

SA_AIS_OK=1
SA_AIS_ERR_LIBRARY=2

#if ! ENV.has_key?('SB_ROOT') then

$: << "%s/lib" % [ENV['AIS_TEST_ROOT']]

require 'test/unit'

class AISTestCase < Test::Unit::TestCase
    def setup()
        require 'AISSys'
        require 'AISTestUtils'

        @utils = AISTestUtils::AISTestUtils.new()

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
