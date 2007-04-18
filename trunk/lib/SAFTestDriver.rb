module SAFTest

require 'SAFTestUtils'
require 'SAFTestAction'
require 'SAFImplementation'

class SAFTestDriverException < Exception
end

class SAFTestDriverThread < SAFTestUtils
    def initialize(params = {})
        super()
        @driver = params[:DRIVER]
        @threadID = params[:THREAD_ID]
        if @threadID == 0
            # We are the main thread
            @socketFile = @driver.getSocketFile()
        else
            @socketFile = "%s/saf_driver_%d_thread_%d.sock" % \
                            [runDir(), @driver.getInstanceID(), @threadID]
        end

        @dispatchThread = false
        @dispatchThreadSpec = nil
    end

    def getThreadID()
        return @threadID
    end

    def getSocketFile()
        return @socketFile
    end

    def setDispatchThread(params = {})
        if params[:DISPATCH_THREAD]
            if not params[:DISPATCH_THREAD_SPEC]
                raise "This thread must know it's specification"
            else
                @dispatchThread = true
                @dispatchThreadSpec = params[:DISPATCH_THREAD_SPEC]
            end
        else
            @dispatchThread = false
            @dispatchThreadSpec = nil
        end
    end

    def dispatchThread?(params = {})
        if not @dispatchThread
            return false
        else
            if params[:DISPATCH_THREAD_SPEC]
                if @dispatchThreadSpec == params[:DISPATCH_THREAD_SPEC]
                    return true
                end
            else
                return true
            end
        end
        
        raise "Shouldn't get here"
    end

    def workerThread?()
        return @dispatchThread == false
    end
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
            driver = SAFTestDriver.new(:NODE => node, 
                                       :DRIVER_LIBS => libPath[1, 
                                                               libPath.length], 
                                       :INSTANCE_ID => id,
                                       :LONG_LIVED => longLived)
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

    #def SAFTestDriver.startNewDriver(node, longLived)
    def SAFTestDriver.startNewDriver(params = {})
        utils = SAFTestUtils.new()
        config = SAFTestConfig.new()
        config.loadFromXML(utils.configXMLFile())

        node = params[:NODE]
        longLived = false
        if params[:LONG_LIVED]
            longLived = true
        end
        libPath = ""
        driverLibs = ["%s/saftest_main_lib.so" % [utils.objDir()]]
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
        instanceID = SAFTestDriver.getNextInstanceID(node, longLived, 
                                                     libPath, config)

        driver = SAFTestDriver.new(:NODE => node, 
                                   :DRIVER_LIBS => libPath[1, libPath.length], 
                                   :INSTANCE_ID => instanceID,
                                   :LONG_LIVED => longLived)
        driver.start()
        return driver
    end 

    # derived classes should have their own version of this that
    # creates a new object of the derived class.
    # I wonder if this function should take a Class object as a parameter?
    def SAFTestDriver.startNewShortLivedDriver(params = {})
        return SAFTestDriver.startNewDriver(params)
    end

public

    def ==(other)
        raise "Huh?" unless SAFTestDriver === other
        return false unless @nodeName == other.getNodeName()
        return false unless @instanceID == other.getInstanceID()
        return true
    end
    alias eql? ==

    def SAFTestDriver.startNewLongLivedDriver(params = {})
        params[:LONG_LIVED] = true
        return SAFTestDriver.startNewDriver(params)
    end

    #def initialize(node, driverLibs, instanceID)
    def initialize(params = {})
        super()
        if params[:NODE]
            @nodeName = params[:NODE].getName()
        elsif params[:NODE_NAME]
            @nodeName = params[:NODE_NAME]
        else
            @nodeName = simpleHostname()
        end

        instanceID = params[:INSTANCE_ID]
        if instanceID <= 0
            raise "Instance ID #{instanceID} invalid, must be > 0"
        else
            @instanceID = instanceID
        end

        @driverPath = "%s/saf_driver" % [objDir()]
        if params[:DRIVER_LIBS]
            @driverLibs = params[:DRIVER_LIBS]
        else 
            @driverLibs = "%s/%s.so" % [objDir(), getName()]
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
        if params[:LONG_LIVED]
            @longLived = true
        end

        # Simply by convention, long lived even numbered drivers will be
        # multi-threaded, and long lived odd numbered drivers will be single
        # threaded
        @mode = "SINGLE_THREADED"
        if @longLived and (instanceID % 2 == 0)
            @mode = "MULTI_THREADED"
        end
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

    def getNodeName()
        return @nodeName
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

    def createResources()
        SAFTestUtils.SUPPORTED_SPECS.each do |spec|
            lower = spec.downcase()
            upper = spec.upcase()
            if @config.valueIsYes('main', "testSpec#{upper}")
                driverLibs << "%s/#{lower}_driver.so" % [objDir()]
            end
        end
    end

    def start()
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
"%s/%s.pid"  % [runDir(), @name]
        cmd = "%s %s --daemon --socket-file %s --run-dir %s --log-file %s --pid-file %s --load-libs %s" % \
              [exportStr, @driverPath, @socketFile, "%s/%s" % [runDir(),'daemon'], 
               @logFile, @pidFile, @driverLibs]
        runAndCheckCommand(cmd, EXPECT_SUCCESS, "Unable to start %s" % \
                           [getName()])
        sleep(1)
        loadPID()
        init(@longLived)
    end

    def stop()
        loadPID()
        runAndCheckCommand("kill -9 %d" % [@pid], EXPECT_SUCCESS,
                           "Unable to kill driver %s with pid %s" % [@name, @pid])
    end

private

    def generateBaseDriverCmd(params = {})
        thread = nil
        if params[:THREAD]
            thread = params[:THREAD]
        else
            thread = getMainThread()
        end
        cmd = "%s --run-dir %s --socket-file %s --load-libs %s --op %s" % \
                 [getDriverPath(), runDir(), thread.getSocketFile(),
                  getDriverLibs(), params[:OP]]
        return cmd
    end

    #def runDriverPrivate(cmd, kvpHash, expectedReturn)
    def runDriverPrivate(params = {})
        cmd = params[:CMD]
        params[:KVP_HASH].each do |key, value|
            cmd = "%s --key \"%s\" --value \"%s\"" % [cmd, 
                                                      key.to_s, value.to_s]
        end
        array = captureCommand(cmd)
        kvp_return = {}
        ret = array[0]
        lines = array[1]
        expectedReturn = getExpectedReturnFromParams(params)
        if expectedReturn != nil and expectedReturn != ret
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

    #def runDriver(op, kvpHash, expectedReturn)
    def runDriver(params = {})
        cmd = generateBaseDriverCmd(params)
        params[:CMD] = cmd
        return runDriverPrivate(params)
    end

    # This version is used when sending a message to the MAIN (base class)
    # level
    #def runDriverNoLibs(op, kvpHash, expectedReturn)
    def runDriverNoLibs(params = {})
        cmd = "%s --run-dir %s --socket-file %s --load-libs %s/saftest_main_lib.so --op %s" % \
                 [getDriverPath(), runDir(), getSocketFile(), objDir(), 
                  params[:OP]]
        params[:CMD] = cmd
        return runDriverPrivate(params)
    end

    def runDriverBG(params = {})
        cmd = generateBaseDriverCmd(params)
        params[:KVP_HASH].each do |key, value|
            cmd = "%s --key \"%s\" --value \"%s\"" % [cmd, key.to_s, value.to_s]
        end

        action = Actions.instance.addAction("background driver")
        action.addCommand(cmd, "background driver")
        action.start()

        return action
    end

    def multiThreaded?()
        if (1..@config.getIntValue('main', 'numLongLivedDrivers')) === @instanceID
            if @instanceID % 2 == 0
                return true
            end
        end

        return false
    end

    def startThreads()
        if not @longLived
            raise "Only Long Lived Drivers have threads"
        end

        kvpHash = {}
        if multiThreaded?()
            numThreads = @config.getIntValue('main', 
                                             'numWorkerThreadsPerDriver')
            SAFTestUtils.SUPPORTED_SPECS.each do |spec|
                lower = spec.downcase()
                upper = spec.upcase()
                if @config.valueIsYes('main', "testSpec#{upper}")
                    numThreads += @config.getIntValue('main', 
                                                      'numDispatchThreadsPerSpec')
                end
            end

            for i in 1..numThreads
                kvpHash['THREAD_SOCKET_FILE'] = 
                    "%s/saf_driver_%d_thread_%d.sock" % \
                        [runDir(), @instanceID, i]
                runDriverNoLibs(:OP => "DRIVER_CREATE_THREAD_REQ", 
                                :KVP_HASH => kvpHash, 
                                :EXPECTED_RETURN => 0)
            end
        end
    end

    def getMainThread()
        # ID 0 represents the main thread
        return SAFTestDriverThread.new(:THREAD_ID => 0,
                                       :DRIVER => self)
    end

    def getThreads(params = {})
        threads = Array.new

        # We should automatically infer this through a virtual function
        if not params[:SPEC]
            raise "You must specify which spec you want threads for"
        end

        # Each threaded driver will have (numWorkerThreadsPerDriver + 
        # NUM_SPECS * numDispatchThreadsPerSpec) threads.  By convention we
        # will just say that all the dispatch drivers come first, and the
        # worker threads come last.
        if multiThreaded?()
            threadID = 1
            SAFTestUtils.SUPPORTED_SPECS.each do |spec|
                upper = spec.upcase()
                if @config.valueIsYes('main', "testSpec#{upper}")
                    for i in 1..@config.getIntValue('main', 
                                                    'numDispatchThreadsPerSpec')
                        thread = SAFTestDriverThread.new(:THREAD_ID => threadID,
                                                         :DRIVER => self)
                        thread.setDispatchThread(:DISPATCH_THREAD => true,
                                                 :DISPATCH_THREAD_SPEC => upper)
                        threadID += 1
                        if params[:SPEC] == upper
                            threads << thread
                        end
                    end
                end
            end

            for i in 1..@config.getIntValue('main', 
                                            'numWorkerThreadsPerDriver')
                thread = SAFTestDriverThread.new(:THREAD_ID => threadID,
                                                 :DRIVER => self)
                thread.setDispatchThread(:DISPATCH_THREAD => false)
                threadID += 1
                threads << thread
            end
        else
            thread = SAFTestDriverThread.new(:THREAD_ID => 0,
                                             :DRIVER => self)
            threads << thread
        end

        return threads
    end

    def getRandomThread()
        threads = getThreads()
        return threads[rand(threads.length())]
    end

    def startSessions()
        SAFTestUtils.SUPPORTED_SPECS.each do |spec|
            lower = spec.downcase()
            upper = spec.upcase()
            if @config.valueIsYes('main', "testSpec#{upper}")
                specDir = "%s/cases/%s" % [ENV['SAFTEST_ROOT'], lower]
                $: << specDir
                driverClass = "%sTestDriver" % [upper]
                require driverClass
                if upper == "CLM"
                    driver = CLMTestDriver.new(:NODE_NAME => @nodeName,
                                               :INSTANCE_ID => @instanceID)
                elsif upper == "LCK"
                    driver = LCKTestDriver.new(:NODE_NAME => @nodeName,
                                               :INSTANCE_ID => @instanceID)
                else
                    raise "Update this to not have hardcoded classes"
                end
                driver.initializeResources()
            end
        end
    end

    def init(longLived)
        kvpHash = {'SAFTEST_DRIVER_LONG_LIVED' => 'FALSE'}
        if longLived == true
            kvpHash['SAFTEST_DRIVER_LONG_LIVED'] = 'TRUE'
        end
        runDriverNoLibs(:OP => "DRIVER_INITIALIZE_REQ", 
                        :KVP_HASH => kvpHash, :EXPECTED_RETURN => 0)
        if longLived
            startThreads()
            startSessions()
        end
    end

    def getStatus()
        kvpHash = {}
        runDriverNoLibs(:OP => "DRIVER_STATUS_REQ", 
                        :KVP_HASH => kvpHash, :EXPECTED_RETURN => 0)
    end

    def waitForFileExists(file)
        waitFor(@@WAIT_INTERVAL, @@WAIT_TIMEOUT, "#{file} to exist") {
            FileTest.exist?(file) and not File.stat(file).zero?
        }
    end

    def getExpectedReturnFromParams(params)
        if params[:EXPECTED_RETURN]
            return params[:EXPECTED_RETURN]
        end
        return SAFTestUtils.SA_AIS_OK
    end
end # class SAFTestDriver

end # module SAFTest
