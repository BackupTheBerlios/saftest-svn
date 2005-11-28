#!/usr/bin/ruby

module SAFTest

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestCase'
require 'SAFTestDriver'

class StopDriversCase < SAFTestCase
    def initialize()
        super()
    end

    def run()
        # !!! This should probably be changed to actually connect to the daemon
        # and ask it to shut down gracefully.
        Dir.foreach(runDir()) {|file|
            if file =~ /saf_driver_\d+.pid/
                pidFile = "%s/%s" % [runDir(), file]
                f = open(File.expand_path(pidFile), 'r')
                pid = f.readline()
                pid.chomp!
                cmd = "kill -9 #{pid}"
                runCommand(cmd)
            end
        }

        passed()
    end
end

test = StopDriversCase.new()
test.run()

end # module
