module AISTestUtils

class AISTestUtils
    def AISTestUtils.SA_AIS_OK
        return 1
    end

    def AISTestUtils.SA_AIS_ERR_LIBRARY
        return 2
    end
    
    def AISTestUtils.SA_AIS_ERR_VERSION
        return 3
    end

    def AISTestUtils.SA_AIS_ERR_INIT
        return 4
    end

    def AISTestUtils.SA_AIS_ERR_TIMEOUT
        return 5
    end

    def AISTestUtils.SA_AIS_ERR_TRY_AGAIN
        return 6
    end

    def AISTestUtils.SA_AIS_ERR_INVALID_PARAM
        return 7
    end

    def AISTestUtils.SA_AIS_ERR_NO_MEMORY
        return 8
    end

    def AISTestUtils.SA_AIS_ERR_BAD_HANDLE
        return 9
    end

    def AISTestUtils.SA_AIS_ERR_BUSY
        return 10 
    end

    def AISTestUtils.SA_AIS_ERR_ACCESS
        return 11
    end

    def AISTestUtils.SA_AIS_ERR_NOT_EXIST
        return 12
    end

    def AISTestUtils.SA_AIS_ERR_NAME_TOO_LONG
        return 13
    end

    def AISTestUtils.SA_AIS_ERR_EXIST
        return 14
    end

    def AISTestUtils.SA_AIS_ERR_NO_SPACE
        return 15
    end

    def AISTestUtils.SA_AIS_ERR_INTERRUPT
        return 16
    end

    def AISTestUtils.SA_AIS_ERR_NAME_NOT_FOUND
        return 17
    end

    def AISTestUtils.SA_AIS_ERR_NO_RESOURCES
        return 18
    end

    def AISTestUtils.SA_AIS_ERR_NOT_SUPPORTED
        return 19
    end

    def AISTestUtils.SA_AIS_ERR_BAD_OPERATION
        return 20
    end

    def AISTestUtils.SA_AIS_ERR_FAILED_OPERATION
        return 21
    end

    def AISTestUtils.SA_AIS_ERR_MESSAGE_ERROR
        return 22
    end

    def AISTestUtils.SA_AIS_ERR_QUEUE_FULL
        return 23
    end

    def AISTestUtils.SA_AIS_ERR_QUEUE_NOT_AVAILABLE
        return 24
    end

    def AISTestUtils.SA_AIS_ERR_BAD_FLAGS
        return 25
    end
    def AISTestUtils.SA_AIS_ERR_TOO_BIG
        return 26
    end

    def AISTestUtils.SA_AIS_ERR_NO_SECTIONS
        return 27
    end

    def mapErrorCodeToString(code)
        case code
            when AISTestUtils.SA_AIS_OK
                return "SA_AIS_OK"

            when AISTestUtils.SA_AIS_ERR_LIBRARY
                return "SA_AIS_ERR_LIBRARY"
        
            when AISTestUtils.SA_AIS_ERR_VERSION
                return "SA_AIS_ERR_VERSION"

            when AISTestUtils.SA_AIS_ERR_INIT
                return "SA_AIS_ERR_INIT"

            when  AISTestUtils.SA_AIS_ERR_TIMEOUT
                return "SA_AIS_ERR_TIMEOUT"

            when AISTestUtils.SA_AIS_ERR_TRY_AGAIN
                return "SA_AIS_ERR_TRY_AGAIN"

            when AISTestUtils.SA_AIS_ERR_INVALID_PARAM
                return "SA_AIS_ERR_INVALID_PARAM"

            when AISTestUtils.SA_AIS_ERR_NO_MEMORY
                return "SA_AIS_ERR_NO_MEMORY"

            when AISTestUtils.SA_AIS_ERR_BAD_HANDLE
                return "SA_AIS_ERR_BAD_HANDLE"

            when AISTestUtils.SA_AIS_ERR_BUSY
                return "SA_AIS_ERR_BUSY"

            when AISTestUtils.SA_AIS_ERR_ACCESS
                return "SA_AIS_ERR_ACCESS"

            when AISTestUtils.SA_AIS_ERR_NOT_EXIST
                return "SA_AIS_ERR_NOT_EXIST"

            when AISTestUtils.SA_AIS_ERR_NAME_TOO_LONG
                return "SA_AIS_ERR_NAME_TOO_LONG"

            when AISTestUtils.SA_AIS_ERR_EXIST
                return "SA_AIS_ERR_EXIST"

            when AISTestUtils.SA_AIS_ERR_NO_SPACE
                return "SA_AIS_ERR_NO_SPACE"

            when AISTestUtils.SA_AIS_ERR_INTERRUPT
                return "SA_AIS_ERR_INTERRUPT"

            when AISTestUtils.SA_AIS_ERR_NAME_NOT_FOUND
                return "SA_AIS_ERR_NAME_NOT_FOUND"

            when AISTestUtils.SA_AIS_ERR_NO_RESOURCES
                return "SA_AIS_ERR_NO_RESOURCES"

            when AISTestUtils.SA_AIS_ERR_NOT_SUPPORTED
                return "SA_AIS_ERR_NOT_SUPPORTED"

            when AISTestUtils.SA_AIS_ERR_BAD_OPERATION
                return "SA_AIS_ERR_BAD_OPERATION"

            when AISTestUtils.SA_AIS_ERR_FAILED_OPERATION
                return "SA_AIS_ERR_FAILED_OPERATION"

            when AISTestUtils.SA_AIS_ERR_MESSAGE_ERROR
                return "SA_AIS_ERR_MESSAGE_ERROR"

            when AISTestUtils.SA_AIS_ERR_QUEUE_FULL
                return "SA_AIS_ERR_QUEUE_FULL"

            when AISTestUtils.SA_AIS_ERR_QUEUE_NOT_AVAILABLE
                return "SA_AIS_ERR_QUEUE_NOT_AVAILABLE"

            when AISTestUtils.SA_AIS_ERR_BAD_FLAGS
                return "SA_AIS_ERR_BAD_FLAGS"

            when AISTestUtils.SA_AIS_ERR_TOO_BIG
                return "SA_AIS_ERR_TOO_BIG"

            when AISTestUtils.SA_AIS_ERR_NO_SECTIONS
                return "SA_AIS_ERR_NO_SECTIONS"
        end
    end

    @@SA_AIS_RELEASE_CODE="B"
    @@SA_AIS_MAJOR_VERSION="01"
    @@SA_AIS_MINOR_VERSION="01"

    def AISTestUtils.SA_AIS_RELEASE_CODE_INTEGER()
        return @@SA_AIS_RELEASE_CODE[0].to_i
    end

    def AISTestUtils.SA_AIS_MAJOR_VERSION_INTEGER()
        return @@SA_AIS_MAJOR_VERSION.to_i
    end

    def AISTestUtils.SA_AIS_MINOR_VERSION_INTEGER()
        return @@SA_AIS_MINOR_VERSION.to_i
    end

    def AISTestUtils.getAISLibVersion()
        return "%s.%s.%s" % \
               [@@SA_AIS_RELEASE_CODE, @@SA_AIS_MAJOR_VERSION,
                @@SA_AIS_MINOR_VERSION]
    end

    def initialize()
        require 'AISSys'

        @aisSys = AISSys::AISSys.new()
        @rootDir = ENV['AIS_TEST_ROOT']
        @runDir = "%s/results/run" % [@rootDir]
        @logDir = "%s/results/log" % [@rootDir]
        @logLevel = 0

        # make sure all output goes to stdout
        # we do this so sb can easily use one pipe to capture all output
        $stderr = $stdout
    end

    def getRunDir()
        return @runDir
    end

    def setLogLevel(level)
        @logLevel = level
    end

    def log(message)
        return if @logLevel == -1
        $stdout.print Time.now.strftime("%b %d %H:%M:%S") + ' ' + message + "\n"
        $stdout.flush
    end

    def passed()
        print "PASSED\n"
        exit(0)
    end

    def failed(message)
        print message + "\n"
        print "FAILED\n"
        exit(1)
    end

    EXPECT_SUCCESS = true
    EXPECT_FAILURE = false

    def fullHostname()
        return @aisSys.fullHostname()
    end

    def simpleHostname()
        return @aisSys.simpleHostname()
    end

    def logCommand(cmd, node, user)
        if nil == node
            node = @aisSys.simpleHostname()
        end
        if nil == user
            user = 'root'
        end
        log("(%s@%s) %s" % [user, node, cmd])
    end

    def runCommand(cmd, node=nil, user=nil)
        logCommand(cmd, node, user)
        exitCode = @aisSys.runCommand(cmd, node, user)
        return exitCode
    end

    def runAndCheckCommand(cmd, expectZeroStatus, errorMessage="", 
                           node=nil, user=nil)
        logCommand(cmd, node, user)
        exitCode = @aisSys.runCommand(cmd, node, user)
        zeroStatus = (exitCode == 0)
        if expectZeroStatus != zeroStatus then
            failed(errorMessage)
        end
    end

    def captureCommand(cmd, node=nil, user=nil)
        logCommand(cmd, node, user)
        array = @aisSys.captureCommand(cmd, node, user)
        return array
    end

end # class

end # module
