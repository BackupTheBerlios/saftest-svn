module SAFTestEngine

require 'xmlparser'
require 'SAFTestUtils'

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]

class SAFTestEngineCase
    def initialize()
        @cmd = ''
        @weight = 0
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
                                print "Current type is #{currentType}\n"
                                print "Current mode is #{currentMode}\n"
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
        @pidfile = "%s/engine.pid" % [runDir()]
        @logfile = "%s/%s.log" % [logDir(), 
                                 @config.getStrValue("main", "testType")]
        @needToHalt = false
        @state = @@INITIALIZING
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
        trap('SIGINT') {
            log("Caught SIGINT.  Shutting down...")
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

    def runCases()
        while (not @needToHalt)
            sleep 1
            log("Woke up")           
        end
    end

end # class

end # module
