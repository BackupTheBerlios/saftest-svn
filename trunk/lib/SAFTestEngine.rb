module SAFTestEngine

require 'SAFTestUtils'
require 'rexml/document'
require 'parsedate'

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]

class SAFTestEngine < SAFTest::SAFTestUtils
    @@INITIALIZING = 'INITIALIZING'
    @@RUNNING_INITIAL_CASES = 'RUNNING_INITIAL_CASES'
    @@RUNNING_MAIN_CASES = 'RUNNING_MAIN_CASES'
    @@RUNNING_FINAL_CASES = 'RUNNING_FINAL_CASES'
    @@HALTING = 'HALTING'
    @@COMPLETED = 'COMPLETED'

    def initialize()
        super()
        @config = SAFTest::SAFTestConfig.new()
        @config.loadFromXML(configXMLFile())
        @bundle = SAFTestBundle.new(bundleFile())
        @cases = @bundle.cases()
        @pidfile = "%s/engine.pid" % [runDir()]
        @statusFile = "%s/engine.status" % [runDir()]
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
            # If weight is 0 then the case is disabled
            if testCase.weight > 0
                tmpArray = Array.new(testCase.weight, testCase)
                tmpArray.each do |tmpCase|
                    @weightedCaseArray << tmpCase
                end
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

    def writeStatus(currentCase)
        f = File.new(@statusFile, "w")
        f.puts "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        f.puts "<SAFTestStatus " + \
              " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " + \
              " xsi:noNamespaceSchemaLocation=\"SAFTestStatus.xsd\" " + \
              " schemaVersion=\"1\">\n"

        f.puts " <SAFTestState>%s</SAFTestState>\n" % [@state]
        f.puts " <SAFTestStartTime>%s</SAFTestStartTime>\n" % [@testStartTime.to_s]
        if currentCase == nil
            f.puts " <SAFTestEndTime>%s</SAFTestEndTime>" % [Time.now().to_s]
        else
            f.puts " <SAFTestCurrentTestCase>\n"
            f.puts "  <name>#{currentCase.name}</name>\n"
            f.puts "  <startTime>%s</startTime>\n" % [Time.now().to_s]
            f.puts " </SAFTestCurrentTestCase>\n"
        end
        f.puts "</SAFTestStatus>\n"
        f.close()
    end

    def displayStatus()
        file = open(File.expand_path(@statusFile), 'r')
        doc = REXML::Document.new(file)

        now = Time.now()
        state = nil
        startTime = nil
        endTime = nil
        currentCaseName = nil
        currentCaseStartTime = nil
        doc.elements.each("SAFTestStatus") {
            |statusElement|
            statusElement.each_element_with_text { |e|
                if e.name == "SAFTestState"
                    state = e.get_text.to_s
                elsif e.name == "SAFTestStartTime"
                    startTime = e.get_text.to_s
                elsif e.name == "SAFTestEndTime"
                    endTime = e.get_text.to_s
                elsif e.name == "SAFTestCurrentTestCase"
                    e.each_element_with_text { |caseElement|
                        if caseElement.name == "name"
                            currentCaseName = caseElement.get_text.to_s
                        elsif caseElement.name == "startTime"
                            currentCaseStartTime = caseElement.get_text.to_s
                        else
                            raise "Unknown element %s" % [caseElement.name]
                        end
                    }
                else
                    raise "Unknown element %s" % [e.name]
                end
            }
        }
        if endTime != nil
            if state != @@COMPLETED
                raise "endTime is non-null only if state is COMPLETED"
            end
            testDuration = Time.local(*ParseDate::parsedate(endTime)) - 
                           Time.local(*ParseDate::parsedate(startTime))
            durationStr = formatSeconds(testDuration.to_s.to_i)
            puts "saftest %s %s in %s" % \
                [@config.getStrValue("main", "testType"), state, durationStr]
        else
            testDuration = now - Time.local(*ParseDate::parsedate(startTime))
            durationStr = formatSeconds(testDuration.to_s.to_i)
            caseDuration = 
                now - Time.local(*ParseDate::parsedate(currentCaseStartTime))
            caseDurationStr = formatSeconds(caseDuration.to_s.to_i)
            puts "saftest %s %s running %s" % \
                [@config.getStrValue("main", "testType"), state, durationStr]
            puts "Current Case: %s running %s" % \
                [currentCaseName, caseDurationStr]
        end
    end

    def runTestCase(testCase, testPhase, node)
        # These test cases are disabled if weight is 0 
        if not testCase.weight > 0
            return SAFTest::SAFTestUtils.NOT_CONFIGURED_EXIT_STATUS
        end

        nodeLog = ""
        if node != nil
            nodeLog = " on #{node}"
        end

        log("BEGIN #{testPhase} #{testCase.name}#{nodeLog} " + 
                "($SAFTEST_ROOT/cases/%s) " % [testCase.command])
        startTime = Time.now()
        writeStatus(testCase)
        exitCode = testCase.run(node)
        endTime = Time.now()
       
        statusStr = SAFTest::SAFTestUtils.EXIT_STATUS_STRINGS[exitCode] 

        elapsedTime = ((endTime - startTime).to_f).to_s
        log("END #{testPhase} #{testCase.name}#{nodeLog} " + 
            "status #{exitCode} %s in #{elapsedTime} seconds\n" % [statusStr])
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

        @state = @@COMPLETED
        @testRealEndTime = Time.now()
        elapsedTime = (@testRealEndTime - @testStartTime).to_i
        writeStatus(nil)
        log("FINISHED in " + formatSeconds(elapsedTime))
    end

end # class

end # module
