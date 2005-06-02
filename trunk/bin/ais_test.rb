#!/usr/bin/ruby

$: << "%s/lib" % [ENV['AIS_TEST_ROOT']]
require 'AISTestUtils'

def usage(msg=nil)
    print msg + "\n" if msg
    print "ais_test.rb [--verbose] \n"
    print "            run [--directory <directory>]\n"
    print "            clean\n"
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

require 'test/unit/ui/console/testrunner'
class AISTestConsoleRunner < Test::Unit::UI::Console::TestRunner
    def test_started(name)
        output_single("BEGIN %s\n" % [name])
    end

    def test_finished(name)
        output_single("END %s\n\n" % [name])
    end
end # class

class AISTestBundle
    require 'test/unit/testsuite'
    require 'AISTestSuite'

    @@suite = Test::Unit::TestSuite.new()

    def self.suite
        return @@suite
    end

    def loadSuites()
        ObjectSpace.each_object(AISTestSuite::AISTestSuite) do |obj|
            $log.print("Object is \"%s\"" % [obj.to_s])
            @@suite << obj.suite
        end
    end

    def loadFromDirectory(path)
        suitePath = path + '/suite.rb'
        require suitePath
    end

    def run()
        #Test::Unit::UI::Console::TestRunner.run(AISTestBundle, 
                                                #Test::Unit::UI::VERBOSE)
        AISTestConsoleRunner.run(AISTestBundle, Test::Unit::UI::VERBOSE)
    end
end # class 

# globals
$log = Log.new()
$utils = AISTestUtils::AISTestUtils.new()
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

if op == 'run'
    bundle = AISTestBundle.new

    cmd = 'mkdir -p %s/results/tmp' % [ENV['AIS_TEST_ROOT']]
    $utils.runAndCheckCommand(cmd, AISTestUtils::AISTestUtils::EXPECT_SUCCESS, 
                              "Error running %s" % [cmd])
    cmd = 'mkdir -p %s/results/run' % [ENV['AIS_TEST_ROOT']]
    $utils.runAndCheckCommand(cmd, AISTestUtils::AISTestUtils::EXPECT_SUCCESS, 
                              "Error running %s" % [cmd])
    cmd = 'mkdir -p %s/results/log' % [ENV['AIS_TEST_ROOT']]
    $utils.runAndCheckCommand(cmd, AISTestUtils::AISTestUtils::EXPECT_SUCCESS, 
                              "Error running %s" % [cmd])
    while (true)
        arg = ARGV.shift
        if arg == "--directory" then
            usage("missing directory argument") if ARGV.length == 0
            dir = ARGV.shift
            bundle.loadFromDirectory(dir)
        end
        break
    end
    bundle.loadSuites()
    bundle.run()
    #engine = AISTestEngine.new(bundle)
    #engine.run()
elsif op == 'clean'
    cmd = 'rm -rf %s/results' % [ENV['AIS_TEST_ROOT']]
    $utils.runAndCheckCommand(cmd, AISTestUtils::AISTestUtils::EXPECT_SUCCESS, 
                                "Error running %s" % [cmd])
else
    usage("unknown operation: %s" % [op])
end

