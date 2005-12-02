module SAFTest

require 'SAFTestUtils'
require 'SAFImplementation'

class SAFTestDriver < SAFTestUtils
    @@nextInstanceID = {} # hash from nodename to next instance id

    def SAFTestDriver.getNextInstanceID(nodeName)
        if not @@nextInstanceID.has_key?(nodeName)
            @@nextInstanceID[nodeName] = 1
        end    

        @@nextInstanceID[nodeName] += 1
        return @@nextInstanceID[nodeName] - 1
    end

    def initialize(node, driverLibs, instanceID)
        super()
        if node != nil
            @nodeName = node.getName()
        else
            @nodeName = nil
        end

        if instanceID == 0
            myInstanceID = SAFTestDriver.getNextInstanceID(@nodeName)
        else
            myInstanceID = instanceID
        end

        @driverPath = "%s/saf_driver" % [objDir()]
        if driverLibs == nil
            @driverLibs = "%s/%s.so" % [objDir(), getName()]
        else 
            @driverLibs = driverLibs
        end

        @instanceID = myInstanceID
        @name = '%s_%d' % [getName(), @instanceID]
        @socketFile = "%s/saf_driver_%d.sock"  % [runDir(), @instanceID]
        @logFile = "%s/%s.log"  % [logDir(), @name]
        @pidFile = "%s/%s.pid"  % [runDir(), @name]
        @pid = nil
        commands_file = "%s/commands.conf" % [implementationDir()]
        @implementation = SAFImplementation.new(commands_file)
        @config = SAFTestConfig.new()
        @config.loadFromXML(configXMLFile())
    end

    def getName()
        return 'saf_driver'
    end

    def getDriverPath()
        return @driverPath
    end

    def getDriverLibs()
        return @driverLibs
    end

    def getSocketFile()
        return @socketFile
    end

    def getLogFile()
        return @logFile
    end

    def getPidFile()
        return @pidFile
    end

    def getImplementation()
        return @implementation
    end

    def runCommand(cmd, node=nil, user=nil)
        return super(cmd, @nodeName, user)
    end

    def runAndCheckCommand(cmd, expectZeroStatus, errorMessage="",
                           node=nil, user=nil)
        return super(
            cmd, expectZeroStatus, errorMessage, @nodeName, user)
    end

    def captureCommand(cmd, node=nil, user=nil)
        return super(cmd, @nodeName, user)
    end

    def start()
        runAndCheckCommand("rm -f %s" % [@logFile], EXPECT_SUCCESS, 
                           "Unable to remove %s" % [@logFile])
        runAndCheckCommand("rm -f %s" % [@pidFile], EXPECT_SUCCESS, 
                           "Unable to remove %s" % [@pidFile])

        cmd = "export CMDLOG_FILE=%s/%s_lib.log; export CONTEXT_LOG_LEVEL=5; %s --daemon --socket-file %s --run-dir %s --log-file %s --pid-file %s --load-libs %s" % \
              [@logDir, @name, @driverPath, @socketFile, runDir(), 
               @logFile, @pidFile, @driverLibs]
        runAndCheckCommand(cmd, EXPECT_SUCCESS, "Unable to start %s" % \
                           [getName()])
        sleep(1)
        cmd = "cat %s" % [@pidFile]
        array = captureCommand(cmd)
        ret, lines = array[0], array[1]
        if ret != 0
            rsafe "%s failed on node %s with status %d" % \
                  [cmd, @node.getName(), ret]
        end
        lines.each do |line|
            if line =~ /^(\d+)/ then
                @pid = $1
                log("Driver pid is %s" % [@pid])
            end
        end
        if nil == @pid
            rsafe "Unable to find a pid for driver %s" % [@name]
        end
    end

    def stop()
        runAndCheckCommand("kill -9 %s" % [@pid], EXPECT_SUCCESS,
                           "Unable to kill driver %s with pid %s" % [@name, @pid])
    end

    def runDriver(op, kvp_hash, expectedReturn)
        cmd = "%s --run-dir %s --socket-file %s --load-libs %s --op %s" % \
                 [getDriverPath(), runDir(), getSocketFile(),
                  getDriverLibs(), op]
        kvp_hash.each do |key, value|
            cmd = "%s --key \"%s\" --value \"%s\"" % [cmd, key.to_s, value.to_s]
        end
        array = captureCommand(cmd)
        kvp_return = {}
        ret = array[0]
        lines = array[1]
        if expectedReturn != ret
            raise "Expected return %s, got %s.  Lines = \"%s\"" % \
                   [mapErrorCodeToString(expectedReturn),
                    mapErrorCodeToString(ret), lines.to_s]
        end
        lines.each do |line|
            if line =~ /(\S+)=(\S+)/
                kvp_return[$1] = $2
            end
        end
        return kvp_return
    end

    def createTestResource()
        array = runDriver("CREATE_TEST_RESOURCE_REQ", {},
                          SAFTestUtils.SA_AIS_OK)
        ret = array[0]
        lines = array[1]
        resourceID = nil
        lines.each do |line|
            if line =~ /^Resource ID=(\d+)/
                resourceID = $1
            end
        end
        if nil == resourceID
            raise "Couldn't find a Resource ID.  Lines = \"%s\"" % [lines.to_s]
        end
        log("new resourceID is %d" % [resourceID])
        return resourceID
    end

end # class SAFTestDriver

end # module SAFTest
