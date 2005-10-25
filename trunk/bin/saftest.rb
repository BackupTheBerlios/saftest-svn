#!/usr/bin/ruby

module SAFTest

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'

def usage(msg=nil)
    print msg + "\n" if msg
    print "saftest.rb [--verbose] \n"
    print "            create [--from-xml <xml file>]\n"
    print "            start\n"
    print "            delete\n"
    exit 1
end

def errExit(msg)
    print msg + "\n"
    exit 1
end

class Log
    def initialize(verbose = false)
        @verbose = verbose
    end

    def setVerbose(verbose) 
        @verbose = verbose 
    end

    def print(msg, verbose = false)
        if (@verbose or @verbose == verbose) then
            $stdout.print msg + "\n"
        end
    end
end # class

#require 'test/unit/ui/console/testrunner'
#class SAFTestConsoleRunner < Test::Unit::UI::Console::TestRunner
    #def test_started(name)
        #output_single("BEGIN %s\n" % [name])
    #end

    #def test_finished(name)
        #output_single("END %s\n\n" % [name])
    #end
#end # class

#class SAFTestBundle
    #require 'test/unit/testsuite'
    #require 'SAFTestSuite'

    #@@suite = Test::Unit::TestSuite.new()

    #def self.suite
        #return @@suite
    #end

    #def loadSuites()
        #ObjectSpace.each_object(SAFTestSuite::SAFTestSuite) do |obj|
            #$log.print("Object is \"%s\"" % [obj.to_s])
            #@@suite << obj.suite
        #end
    #end

    #def loadFromDirectory(path)
        #suitePath = path + '/suite.rb'
        #require suitePath
    #end

    #def run()
        #SAFTestConsoleRunner.run(SAFTestBundle, Test::Unit::UI::VERBOSE)
    #end
#end # class 

class SAFTestConfig
    def initialize(verbose = false)
        # Hash from Section Name to Config Hash
        @config = {}
    end

    def insertConfigValue(section, key, value)
        if not @config.has_key?(section)
            @config[section] = {}
        end
        @config[section][key] = value
    end

    def promptArray(section, key, label, optionArray, defaultOption)
        optionString = ''
        optionArray.each do |option|
            optionString = '%s|%s' % [optionString, option]
        end
        while(true)
            print "%s (%s) [%s]: " % [label, 
                                      optionString.slice(1, 
                                                         optionString.length),
                                      defaultOption]
            answer = gets
            answer.chomp!
            if answer == ''
                answer = defaultOption
            end

            if optionArray.include?(answer)
                insertConfigValue(section, key, answer)
                break
            else
                print "Invalid answer: \"%s\"\n" % [answer]
            end
        end
    end

    def promptYesNo(section, key, label, defaultIsYes)
        while(true)
            if defaultIsYes
                defaultOption = 'yes'
            else
                defaultOption = 'no'
            end

            print "%s (yes/no) [%s]: " % [label, defaultOption]
            answer = gets
            answer.chomp!
            if answer == ''
                answer = defaultOption
            end

            if answer.downcase()[0] == 'y'[0]
                insertConfigValue(section, key, 'yes')
                break
            elsif answer.downcase()[0] == 'n'[0]
                insertConfigValue(section, key, 'no')
                break
            else
                print "Invalid answer: \"%s\"\n" % [answer]
            end
        end
    end

    def save()
        f = File.new("%s/saftest.xml" % [$utils.confDir], "w")
        f.puts "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        f.puts "<SAFTestConfig " + \
              " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " + \
              " xsi:noNamespaceSchemaLocation=\"SAFTestConfig.xsd\" " + \
              " schemaVersion=\"1\">\n"

        f.puts "<SAFTestConfigSectionList>\n"
        @config.keys().each do |section|
            f.puts " <SAFTestConfigSection>\n"
            f.puts "  <name>%s</name>\n" % [section]
            f.puts "  <SAFTestConfigEntryList>\n"
            @config[section].keys().each do |key|
                f.puts "   <SAFTestConfigEntry>\n"
                f.puts "    <name>%s</name>\n" % [key]
                f.puts "    <value>%s</value>\n" % [@config[section][key]]
                f.puts "   </SAFTestConfigEntry>\n"
            end
            f.puts "  </SAFTestConfigEntryList>\n"
            f.puts " </SAFTestConfigSection>\n"
        end
        f.puts "</SAFTestConfigSectionList>\n"
    end
end

if not ENV.has_key?('SAFTEST_ROOT')
    errExit("You must define a SAFTEST_ROOT environment variable")
end

# globals
$log = Log.new()
$utils = SAFTestUtils.new()
op = nil

while (true)
    opt = ARGV.shift;

    if opt == '--verbose' then
        $log.setVerbose(true)
    elsif opt != nil and opt[1,1] == '-' then
        usage("Invalid option: #{opt}")
    else
        op = opt
        if op == nil
            usage()
        end
        break;
    end
end

if op == 'create'
    workDir = $utils.workDir
    workDirExists = true
    begin
        entries = Dir.entries(workDir)
    rescue SystemCallError
        workDirExists = false
    end

    if workDirExists
        errExit("Directory %s already exists" % [workDir])
    else
        $utils.makeWorkDirs()
    end

    config = SAFTestConfig.new
    config.promptArray('main', 'testType', 'Test Type', 
                       ['conformance', 'functional'], 'functional')
    config.promptYesNo('main', 'testCLM', 'Test CLM', true)
    config.promptYesNo('main', 'testLCK', 'Test LCK', false)
    config.promptYesNo('main', 'testMSG', 'Test MSG', false)
    config.save()
#elsif op == 'run'
    #bundle = SAFTestBundle.new

    #cmd = 'mkdir -p %s/results/tmp' % [ENV['SAFTEST_ROOT']]
    #$utils.runAndCheckCommand(cmd, SAFTestUtils::SAFTestUtils::EXPECT_SUCCESS, 
                              #"Error running %s" % [cmd])
    #cmd = 'mkdir -p %s/results/run' % [ENV['SAFTEST_ROOT']]
    #$utils.runAndCheckCommand(cmd, SAFTestUtils::SAFTestUtils::EXPECT_SUCCESS, 
                              #"Error running %s" % [cmd])
    #cmd = 'mkdir -p %s/results/log' % [ENV['SAFTEST_ROOT']]
    #$utils.runAndCheckCommand(cmd, SAFTestUtils::SAFTestUtils::EXPECT_SUCCESS, 
                              #"Error running %s" % [cmd])
    #while (true)
        #arg = ARGV.shift
        #if arg == "--directory" then
            #usage("missing directory argument") if ARGV.length == 0
            #dir = ARGV.shift
            #bundle.loadFromDirectory(dir)
        #end
        #break
    #end
    #bundle.loadSuites()
    #bundle.run()
    #engine = SAFTestEngine.new(bundle)
    #engine.run()
elsif op == 'delete'
    cmd = 'rm -rf %s' % [$utils.workDir]
    $utils.runAndCheckCommand(cmd, SAFTestUtils::EXPECT_SUCCESS, 
                              "Error running %s" % [cmd])
else
    usage("unknown operation: %s" % [op])
end

end # module SAFTest
