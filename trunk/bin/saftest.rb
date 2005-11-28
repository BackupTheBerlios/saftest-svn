#!/usr/bin/ruby

module SAFTest

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'SAFTestConfig'
require 'SAFTestEngine'

class Log

    def Log.usage(msg = nil)
        print msg + "\n" if msg
        print "saftest.rb [--verbose] \n"
        print "            create [--from-xml <xml file>]\n"
        print "            start\n"
        print "            stop\n"
        print "            delete\n"
        exit 1
    end

    def Log.errExit(msg)
        print msg + "\n"
        exit 1
    end

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

if not ENV.has_key?('SAFTEST_ROOT')
    Log.errExit("You must define a SAFTEST_ROOT environment variable")
end

# globals
$log = Log.new()
$utils = SAFTestUtils.new()
op = nil

while (true)
    opt = ARGV.shift;

    if opt == '--verbose' then
        $log.setVerbose(true)
    elsif opt == '--help' then
        Log.usage()
    elsif opt != nil and opt[1,1] == '-' then
        Log.usage("Invalid option: #{opt}")
    else
        op = opt
        if op == nil
            Log.usage()
        end
        break
    end
end

if op == 'create'
    xmlConfig = nil
    while (true)
        opt = ARGV.shift;

        if opt == '--from-xml' then
            opt = ARGV.shift
            xmlConfig = opt 
        elsif opt != nil and opt[1,1] == '-' then
            Log.usage("Invalid option: #{opt}")
        else
            break
        end
    end

    workDir = $utils.workDir
    workDirExists = true
    begin
        entries = Dir.entries(workDir)
    rescue SystemCallError
        workDirExists = false
    end

    if workDirExists
        Log.errExit("Directory %s already exists" % [workDir])
    else
        $utils.makeWorkDirs()
    end

    if xmlConfig != nil
        config = SAFTestConfig.new
        config.loadFromXML(xmlConfig)
    else
        implementations = []
        begin
            Dir.new("%s/implementation" % [$utils.rootDir]).each do |d|
                begin
                    # "d" must be a directory.  See if it starts with a letter
                    # or number
                    if d =~ /^[A-z0-9].*/
                        implementations << d
                    end
                rescue SystemCallError
                    print "error"
                end
            end
        rescue SystemCallError
            Log.errExit("%s/implementation doesn't exist" % [$utils.rootDir])
        end

        if implementations.length == 0
            Log.errExit("%s/implementation has no entries" % [$utils.rootDir])
        end

        config = SAFTestConfig.new
        config.promptArray('main', 'testType', 'Test Type', 
                           ['conformance', 'functional'], 'functional')
        config.promptArray('main', 'implementation', 'Implementation', 
                           implementations, implementations[0])
        config.promptInt('main', 'numLongLivedDrivers', 
                         'Number of Long-Lived Drivers', 1, 10, 3)
        SAFTestUtils.SUPPORTED_SPECS.each do |spec|
            config.promptYesNo('main', "testSpec#{spec}", "Test #{spec}", true)
        end
    end
    config.save()

    # Validate the implementation files are correct
    commandsFile = "%s/implementation/%s/commands.conf" % 
                   [$utils.rootDir, 
                    config.getStrValue('main', 'implementation')]
    f = open(File.expand_path(commandsFile), 'r')

    # Copy the appropriate bundle file into the ork directory
    cmd = "cp -f %s/conf/%s_bundle.xml %s" % 
          [$utils.rootDir, 
           config.getStrValue('main', 'testType'),
           $utils.bundleFile]
    $utils.runAndCheckCommand(cmd, SAFTestUtils::EXPECT_SUCCESS,
                              "Error running #{cmd}")

    # Copy the implementation files into the work directory
    cmd = "cp -a %s/implementation/%s/* %s" % 
          [$utils.rootDir, 
           config.getStrValue('main', 'implementation'),
           $utils.implementationDir]
    $utils.runAndCheckCommand(cmd, SAFTestUtils::EXPECT_SUCCESS,
                              "Error running #{cmd}")
elsif op == 'start'
    engine = SAFTestEngine::SAFTestEngine.new()
    engine.start()
elsif op == 'stop'
    engine = SAFTestEngine::SAFTestEngine.new()
    engine.stop()
elsif op == 'delete'
    cmd = 'rm -rf %s' % [$utils.workDir]
    $utils.runAndCheckCommand(cmd, SAFTestUtils::EXPECT_SUCCESS, 
                              "Error running %s" % [cmd])
else
    Log.usage("unknown operation: %s" % [op])
end

end # module SAFTest
