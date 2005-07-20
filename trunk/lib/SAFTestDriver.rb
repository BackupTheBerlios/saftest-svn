module SAFTestDriver

require 'SAFTestUtils'
require 'SAFImplementation'

class SAFTestDriver < SAFTestUtils::SAFTestUtils
    def initialize(node, driverLibs, instanceId)
        super()
        @driverPath = ENV['SAFTEST_ROOT'] + '/driver/saf_driver'
        @driverLibs = driverLibs
        @instanceId = instanceId
        @name = '%s_%d' % [getName(), @instanceId]
        @socketFile = "%s/%s.sock"  % [getRunDir(), @name]
        @logFile = "%s/%s.log"  % [@logDir, @name]
        @pidFile = "%s/%s.pid"  % [getRunDir(), @name]
        @pid = nil
        if node != nil
            @nodeName = node.getName()
        else
            @nodeName = nil
        end
        commands_file = ENV['SAFTEST_ROOT'] + '/conf/cluster_commands.conf'
        @implementation = SAFImplementation::SAFImplementation.new(commands_file)
    end

    def killAllDrivers()
        captureCommand("killall -9 saf_driver")
    end

    def getName()
        rsafe 'Subclass must override'
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
              [@logDir, @name, @driverPath, @socketFile, @runDir, 
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

end # class SAFSys

end # module SAFSys
