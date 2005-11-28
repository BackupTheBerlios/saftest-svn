module SAFTestEngine

require 'xmlparser'
require 'SAFTestUtils'

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]

class SAFTestEngineCase < SAFTest::SAFTestUtils
    def initialize()
        super()
        @name = ''
        @cmd = ''
        @weight = 0
    end

    def name()
        return @name
    end

    def name=(name)
        @name = name
    end

    def command()
        return @cmd
    end

    def command=(cmd)
        @cmd = cmd
    end

    def weight()
        return @weight
    end

    def weight=(weight)
        @weight = weight
    end

    def run(node)
        fullCmd = "%s/cases/%s" % [ENV['SAFTEST_ROOT'], @cmd]
        exitCode = @safSys.runCommand(fullCmd, node) do |line|
            line.chomp!
            log(line)
        end
        return exitCode
    end
end

class SAFTestBundle

    def initialize(xmlPath)
        @cases = {}
        @mainMode = nil # sequential or random
        loadFromXML(xmlPath)
    end

    def getMainMode()
        return @mainMode
    end

    def cases()
        return @cases
    end

    def loadFromXML(xmlPath)
        f = open(File.expand_path(xmlPath), 'r')
        xmlLines = f.readlines()
        f.close
        xml = ''
        xmlLines.each do |line|
            if line =~ /^<\?xml.*/ then
                next
            end
            xml += line
        end

        currentCase = nil
        currentType = nil
        currentMode = nil
        currentElement = nil
        parser = XML::Parser.new
        begin
            parser.parse(xml) do |type, name, data|
                case type
                    when XML::Parser::START_ELEM
                        case name
                            when ''
                                raise 'Empty Start Tag?'
                            when 'SAFTestCaseList'
                                currentType = data['type']
                                currentMode = data['mode']
                                if currentType == 'main'
                                    if currentMode != 'sequential' and
                                       currentMode != 'random'
                                        raise "Invalid mode #{currentMode}"
                                    end
                                    @mainMode = currentType
                                end
                                @cases[currentType] = []
                            when 'SAFTestCase'
                                # No attributes on these tags
                                currentCase = SAFTestEngineCase.new
                                currentElement = name
                            else
                                currentElement = name
                        end

                    when XML::Parser::CDATA
                        case currentElement
                            when 'name'
                                currentCase.name = data
                            when 'cmd'
                                currentCase.command = data
                            when 'weight'
                                currentCase.weight = data.to_i
                        end

                    when XML::Parser::END_ELEM
                        case name
                            when 'SAFTestCase'
                                @cases[currentType] << currentCase
                                currentCase = nil
                        end
                        currentElement = nil
                end
            end
        rescue XMLParserError
            line = parser.line
            raise "XML parse error #{$!}: LINE #{line} FROM #{xml}\n"
        end
    end       

    def display()
        print "Initial Cases:\n"
        @cases['initial'].each do |c|
            print "  Cmd: %s (%d)\n" % [c.command(), c.weight()]
        end
        print "\nMain Cases (type = %s):\n" % [@mainMode]
        @cases['main'].each do |c|
            print "  Cmd: %s (%d)\n" % [c.command(), c.weight()]
        end
        print "\nFinal Cases:\n"
        @cases['final'].each do |c|
            print "  Cmd: %s (%d)\n" % [c.command(), c.weight()]
        end
    end
end # class

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
        @testEndTime = nil

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
        while (not @needToHalt)
            testCase = selectRandomMainTestCase()
            exitCode = runTestCase(testCase, 'main', nil)
        end

        @state = @@RUNNING_FINAL_CASES
        @cases['final'].each do |testCase|
            exitCode = runTestCase(testCase, 'final', nil)
        end

        @testEndTime = Time.now()
        elapsedTime = (@testEndTime - @testStartTime).to_i
        log("FINISHED in " + formatSeconds(elapsedTime))
    end

end # class

end # module
