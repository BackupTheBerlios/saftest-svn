module SAFTestEngine

require 'xmlparser'
require 'SAFTestUtils'

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]

class SAFTestEngine < SAFTest::SAFTestUtils
    @@INITIALIZING = 'INITIALIZING'
    @@RUNNING_INITIAL_CASES = 'RUNNING_INITIAL_CASES'
    @@RUNNING_MAIN_CASES = 'RUNNING_MAIN_CASES'
    @@RUNNING_FINAL_CASES = 'RUNNING_FINAL_CASES'
    @@HALTING = 'HALTING'

    def initialize()
        super()
        @config = SAFTest::SAFTestConfig.new()
        @config.loadFromXML(configXMLFile())
        @bundle = SAFTestBundle.new(bundleFile())
        @cases = @bundle.cases()
        @pidfile = "%s/engine.pid" % [runDir()]
        @logfile = "%s/%s.log" % [logDir(), 
                                 @config.getStrValue("main", "testType")]
        @needToHalt = false
        @state = @@INITIALIZING
        @testStartTime = nil
        @testEndTime = nil # The time the test SHOULD end
        @testRealEndTime = nil # The time the test ACTUALLY ended

        # weightedCaseArray will be an array where each test case has as many
        # entries in the array as it's weight.  If case A has weight 2, it will
        # have 2 entries in weightedCaseArray and if case B has weight 4, it 
        # will have 4 entries.  Selected a random test case means just picking
        # a random number in the rage [0, totalWeight) and using that as an
        # index into weightedCaseArray.

        @weightedCaseArray = Array.new
        setupMainTestCases()
    end

    def setupMainTestCases()
        @cases['main'].each do |testCase|
            tmpArray = Array.new(testCase.weight, testCase)
            tmpArray.each do |tmpCase|
                @weightedCaseArray << tmpCase
            end
        end
    end

    def selectRandomMainTestCase()
        return @weightedCaseArray[rand(@weightedCaseArray.length)]
    end

    def daemonize()
        exit if fork
        # Become session leader.
        Process.setsid
        # Zap session leader. See [1].
        exit if fork

        # Release old working directory.
        Dir.chdir "/"

        # Ensure sensible umask. Adjust as needed.
        #File.umask 0000

        STDIN.reopen "/dev/null"
        STDOUT.reopen @logfile, "a"
        STDERR.reopen STDOUT

        # Write our pidfile
        f = File.new(@pidfile, "w")
        f.puts $$
        f.close
    end 

    def start()
        @testStartTime = Time.now()

        # We only have a test duration if the mainMode is random
        if @bundle.getMainMode() == 'random' then
            duration = @config.getStrValue('main', 'testDuration')
            if duration =~ /^(\d+)(\S+)/
                number = $1.to_i
                unit = $2
            else
                raise "Unparseable test duration string \"#{duration}\""
            end

            durationSecs = 0
            if unit == 'm'
                durationSecs = number * 60
            elsif unit == 'h'
                durationSecs = number * 60 * 60
            else
                raise "Unknown time unit \"#{unit}\"\n"
            end

            @testEndTime = @testStartTime + durationSecs
        end

        trap('SIGINT') {
            log("ENGINE: Caught SIGINT.  Shutting down...")
            @needToHalt = true
        }

        daemonize()
        runCases()
    end

    def stop()
        f = open(File.expand_path(@pidfile), 'r')
        pid = f.readline()
        pid.chomp!
        pid = pid.to_i
        print "Sending SIGINT to pid %d\n" % [pid]
        Process.kill("SIGINT", pid)
    end

    def checkDuration()
        now = Time.now()
        if now >= @testEndTime
            @needToHalt = true
        end
    end

    def runTestCase(testCase, testPhase, node)
        nodeLog = ""
        if node != nil
            nodeLog = " on #{node}"
        end

        log("BEGIN #{testPhase} #{testCase.name}#{nodeLog}")

        startTime = Time.now()
        exitCode = testCase.run(node)
        endTime = Time.now()

        elapsedTime = ((endTime - startTime).to_f).to_s
        log("END #{testPhase} #{testCase.name}#{nodeLog} " + 
            "status #{exitCode} in #{elapsedTime} seconds\n")
        return exitCode
    end

    def runCases()
        @state = @@RUNNING_INITIAL_CASES
        @cases['initial'].each do |testCase|
            exitCode = runTestCase(testCase, 'initial', nil)
        end

        @state = @@RUNNING_MAIN_CASES
        if @bundle.getMainMode() == 'sequential' then
            @cases['main'].each do |testCase|
                exitCode = runTestCase(testCase, 'main', nil)
            end
        else
            if @bundle.getMainMode() != 'random' then
                raise "mainMode must be 'sequential' or 'random', but is %s" \
                      % [@bundle.getMainMode()]
            end

            while (not @needToHalt)
                testCase = selectRandomMainTestCase()
                exitCode = runTestCase(testCase, 'main', nil)
                checkDuration()
            end
        end

        @state = @@RUNNING_FINAL_CASES
        @cases['final'].each do |testCase|
            exitCode = runTestCase(testCase, 'final', nil)
        end

        @testRealEndTime = Time.now()
        elapsedTime = (@testRealEndTime - @testStartTime).to_i
        log("FINISHED in " + formatSeconds(elapsedTime))
    end

end # class

end # module
