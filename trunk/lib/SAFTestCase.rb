module SAFTest

require 'SAFTestUtils'
require 'SAFTestConfig'
require 'SAFImplementation'

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
        commands_file = "%s/commands.conf" % [@utils.implementationDir()]
        @implementation = SAFImplementation.new(commands_file)
        @params = {}

        # Test case parameters take the form:
        #  --key1=value1 --key2=value2 ... --keyN=valueN
        # 
        # Inserted into the hash would be keyN,valueN
       
        while (true)
            opt = ARGV.shift
            if nil == opt
                break
            end
            if opt =~ /--(\S+)=(\S+)/
                @params[$1] = $2
            end
        end
    end

    def getParam(key)
        return @params[key]
    end

    def runDir()
        return @utils.runDir()
    end

    def tmpFile(fileName)
        return "%s/%s.%d" % [@utils.tmpDir(), fileName, $$]
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
