#!/usr/bin/ruby

module SAFTest

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestCase'
require 'SAFTestDriver'

class StartDriversCase < SAFTestCase

    def initialize()
        super()
    end

    def run()
        libPath = ""
        driverLibs = []
        if @config.valueIsYes('main', 'testCLM')
            driverLibs << "cases/clm/driver/clm_driver.so"
        end
        if @config.valueIsYes('main', 'testLCK')
            driverLibs << "cases/lck/driver/lck_driver.so"
        end
        if @config.valueIsYes('main', 'testMSG')
            driverLibs += "cases/msg/driver/msg_driver.so"
        end
        driverLibs.each do |lib|
            libPath += ",%s/%s" % [ENV['SAFTEST_ROOT'], lib] 
        end
        print "Num Drivers: %d\n" % [@config.getIntValue('main', 'numLongLivedDrivers')]
        @implementation.getCluster().getNodes().each do |node|
            ndx = 0
            while ndx < @config.getIntValue('main', 'numLongLivedDrivers')
                driver = SAFTestDriver.new(node, libPath[1, libPath.length], 0)
                driver.start()
                ndx += 1
            end
        end
        passed()
    end
end

test = StartDriversCase.new()
test.run()

end # module
