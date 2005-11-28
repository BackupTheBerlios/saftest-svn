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
        SAFTestUtils.SUPPORTED_SPECS.each do |spec|
            lower = spec.downcase()
            upper = spec.upcase()
            if @config.valueIsYes('main', "testSpec#{upper}")
                driverLibs << "cases/#{lower}/driver/#{lower}_driver.so"
            end
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
