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
        driverLib = "%s/AIS-clm-%s/driver/clm_driver.so,%s/AIS-lck-%s/driver/lck_driver.so,%s/AIS-msg-%s/driver/msg_driver.so" % \
                    [ENV['SAFTEST_ROOT'],
                     SAFTestUtils.getAISLibVersion(),
                     ENV['SAFTEST_ROOT'],
                     SAFTestUtils.getAISLibVersion(),
                     ENV['SAFTEST_ROOT'],
                     SAFTestUtils.getAISLibVersion()]

        ndx = 0
        print "Num Drivers: %d\n" % [@config.getIntValue('main', 'numLongLivedDrivers')]
        while ndx < @config.getIntValue('main', 'numLongLivedDrivers')
            driver = SAFTestDriver.new(nil, driverLib, 0)
            driver.start()
            ndx += 1
        end
        passed()
    end
end

test = StartDriversCase.new()
test.run()

end # module
