module SAFTest

require 'SAFTestUtils'
require 'SAFTestConfig'

SA_SAF_OK=1
SA_SAF_ERR_LIBRARY=2

#if ! ENV.has_key?('SB_ROOT') then

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]

class SAFTestCase
    def initialize()

        # make sure all output goes to stdout
        # we do this so sb can easily use one pipe to capture all output
        $stderr = $stdout

        @utils = SAFTestUtils.new()
        @config = SAFTestConfig.new()
        @config.loadFromXML(@utils.configXMLFile())
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
        @utils.passed()
    end

    def failed(message)
        @utils.failed(message)
    end

    def skipped(message)
        @utils.skipped(message)
    end

    def notConfigured(message)
        @utils.notConfigured(message)
    end

    def logCommand(cmd, node, user)
        @utils.logCommand(cmd, node, user)
    end

    def runCommand(cmd, node=nil, user=nil)
        #@utils.logCommand(cmd, node, user)
        return @utils.runCommand(cmd, node, user)
    end

    def runAndCheckCommand(cmd, expectZeroStatus, errorMessage="", 
                           node=nil, user=nil)
        #@utils.logCommand(cmd, node, user)
        @utils.runAndCheckCommand(cmd, expectZeroStatus, errorMessage, 
                                  node, user)
    end

    def captureCommand(cmd, node=nil, user=nil)
        #@utils.logCommand(cmd, node, user)
        return @utils.captureCommand(cmd, node, user)
    end

end # class

end # module
