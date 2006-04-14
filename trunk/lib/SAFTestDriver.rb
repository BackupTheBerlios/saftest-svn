module SAFTest

require 'SAFTestUtils'
require 'SAFTestAction'
require 'SAFImplementation'

class SAFTestDriverException < Exception
end

class SAFTestDriver < SAFTestUtils
    # This is the timeout we use when waiting for things to happen
    @@WAIT_INTERVAL = 1
    @@WAIT_TIMEOUT = 30
    @@MAX_NUM_SHORT_LIVED_DRIVERS = 5

protected

    def SAFTestDriver.getNextInstanceID(node, longLived, libPath, config)
        nextInstanceID = 0
        maxLongLivedID = config.getIntValue('main', 'numLongLivedDrivers')
        maxShortLivedID = maxLongLivedID + 1 + @@MAX_NUM_SHORT_LIVED_DRIVERS

        range = nil
        if longLived
            range = 1..maxLongLivedID
        else
            range = maxLongLivedID + 1..maxShortLivedID
        end

        for id in range
            if nextInstanceID != 0
                break
            end
            driver = SAFTestDriver.new(node, 
                                       libPath[1, libPath.length], 
                                       id)
            begin
                driver.getStatus()
            rescue SAFTestDriverException => e
                nextInstanceID = id
            end 
        end

        if nextInstanceID == 0
            raise "Unable to find an unused driver ID"
        end
        return nextInstanceID
    end

    def SAFTestDriver.startNewDriver(node, longLived)
        utils = SAFTestUtils.new()
        config = SAFTestConfig.new()
        config.loadFromXML(utils.configXMLFile())

        libPath = ""
        driverLibs = []
        SAFTestUtils.SUPPORTED_SPECS.each do |spec|
            lower = spec.downcase()
            upper = spec.upcase()
            if config.valueIsYes('main', "testSpec#{upper}")
                driverLibs << "%s/#{lower}_driver.so" % [utils.objDir()]
            end
        end
        driverLibs.each do |lib|
            libPath += ",%s" % [lib]
        end
        instanceID = SAFTestDriver.getNextInstanceID(node, longLived, libPath,
                                                     config)

        driver = SAFTestDriver.new(node, libPath[1, libPath.length], instanceID)
        driver.start(longLived)
        return driver
    end 

    # derived classes should have their own version of this that
    # creates a new object of the derived class.
    # I wonder if this function should take a Class object as a parameter?
    def SAFTestDriver.startNewShortLivedDriver(node)
        return SAFTestDriver.startNewDriver(node, false)
    end

public

    def SAFTestDriver.startNewLongLivedDriver(node)
        return SAFTestDriver.startNewDriver(node, true)
    end

    def initialize(node, driverLibs, instanceID)
        super()
        if node != nil
            @nodeName = node.getName()
        else
            @nodeName = simpleHostname()
        end

        if instanceID <= 0
            raise "Instance ID #{instanceID} invalid, must be > 0"
        else
            @instanceID = instanceID
        end

        @driverPath = "%s/saf_driver" % [objDir()]
        if driverLibs == nil
            @driverLibs = "%s/%s.so" % [objDir(), getName()]
        else 
            @driverLibs = driverLibs
        end

        @name = "saf_driver_#{instanceID}"
        @socketFile = "%s/saf_driver_%d.sock"  % [runDir(), @instanceID]
        @logFile = "%s/%s.log"  % [logDir(), @name]
        @pidFile = "%s/%s.pid"  % [runDir(), @name]
        @pid = nil
        @implementation = SAFImplementation.new()
        @config = SAFTestConfig.new()
        @config.loadFromXML(configXMLFile())
        @longLived = false
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

    def getInstanceID()
        return @instanceID
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

    def loadPID()
        return if @pid != nil

        cmd = "cat %s" % [@pidFile]
        array = captureCommand(cmd)
        ret, lines = array[0], array[1]
        if ret != 0
            raise "%s failed on node %s with status %d" % \
                  [cmd, @nodeName, ret]
        end
        lines.each do |line|
            if line =~ /^(\d+)/ then
                @pid = $1
                log("Driver pid is %s" % [@pid])
            end
        end
        if nil == @pid
            raise "Unable to find a pid for driver %s" % [@name]
        end
    end

    def start(longLived)
        runAndCheckCommand("rm -f %s" % [@logFile], EXPECT_SUCCESS, 
                           "Unable to remove %s" % [@logFile])
        runAndCheckCommand("rm -f %s" % [@pidFile], EXPECT_SUCCESS, 
                           "Unable to remove %s" % [@pidFile])

        exportStr = ''
        driverLogEnvVar = @implementation.getDriverLogEnvironmentVariable()
        if driverLogEnvVar != nil
            exportStr += "export %s=%s/%s_lib.log; " % \
                         [driverLogEnvVar, logDir(), @name]
        end

        driverEnvVars = @implementation.getDriverEnvironmentVariables()
        driverEnvVars.keys().each do |key|
            exportStr += "export %s=%s; " % [key, driverEnvVars[key]]
        end

        cmd = "%s %s --daemon --socket-file %s --run-dir %s --log-file %s --pid-file %s --load-libs %s" % \
              [exportStr, @driverPath, @socketFile, runDir(), 
               @logFile, @pidFile, @driverLibs]
        runAndCheckCommand(cmd, EXPECT_SUCCESS, "Unable to start %s" % \
                           [getName()])
        sleep(1)
        loadPID()
        init(longLived)
    end

    def stop()
        loadPID()
        runAndCheckCommand("kill -9 %d" % [@pid], EXPECT_SUCCESS,
                           "Unable to kill driver %s with pid %s" % [@name, @pid])
    end

private

    def generateBaseDriverCmd(op)
        cmd = "%s --run-dir %s --socket-file %s --load-libs %s --op %s" % \
                 [getDriverPath(), runDir(), getSocketFile(),
                  getDriverLibs(), op]
        return cmd
    end

    def runDriverPrivate(cmd, kvpHash, expectedReturn)
        kvpHash.each do |key, value|
            cmd = "%s --key \"%s\" --value \"%s\"" % [cmd, key.to_s, value.to_s]
        end
        array = captureCommand(cmd)
        kvp_return = {}
        ret = array[0]
        lines = array[1]
        if expectedReturn != ret
            raise SAFTestDriverException.new(),
                  "Expected return %s, got %s.  Lines = \"%s\"" % \
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

public

    def runDriver(op, kvpHash, expectedReturn)
        cmd = generateBaseDriverCmd(op)
        return runDriverPrivate(cmd, kvpHash, expectedReturn)
    end

    # This version is used when sending a message to the MAIN (base class)
    # level
    def runDriverNoLibs(op, kvpHash, expectedReturn)
        cmd = "%s --run-dir %s --socket-file %s --op %s" % \
                 [getDriverPath(), runDir(), getSocketFile(), op]
        return runDriverPrivate(cmd, kvpHash, expectedReturn)
    end

    def runDriverBG(op, kvpHash)
        cmd = generateBaseDriverCmd(op)
        kvpHash.each do |key, value|
            cmd = "%s --key \"%s\" --value \"%s\"" % [cmd, key.to_s, value.to_s]
        end

        action = Actions.instance.addAction("background driver")
        action.addCommand(cmd, "background driver")
        action.start()

        return action
    end

    def init(longLived)
        kvpHash = {'SAFTEST_DRIVER_LONG_LIVED' => 'FALSE'}
        if longLived == true
            kvpHash['SAFTEST_DRIVER_LONG_LIVED'] = 'TRUE'
        end
        runDriverNoLibs("DRIVER_INITIALIZE_REQ", kvpHash, 0)
    end

    def getStatus()
        kvpHash = {}
        runDriverNoLibs("DRIVER_STATUS_REQ", kvpHash, 0)
    end

    def waitForFileExists(file)
        waitFor(@@WAIT_INTERVAL, @@WAIT_TIMEOUT, "#{file} to exist") {
            FileTest.exist?(file) and not File.stat(file).zero?
        }
    end

end # class SAFTestDriver

end # module SAFTest
