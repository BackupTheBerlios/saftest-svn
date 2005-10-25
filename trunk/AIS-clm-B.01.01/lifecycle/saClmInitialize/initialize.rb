#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << clmDir

class InitializeCase < Test::Unit::TestCase
    require 'CLMTestDriver'

    def test_run()
        CLMTestDriver::CLMTestDriver.getLongLivedDrivers(nil).each do |d|
            resourceID = d.createTestResource()
            d.init(resourceID, true, true, "SA_DISPATCH_ALL",
                   SAFTestUtils::SAFTestUtils.SA_AIS_OK)
        end
    end
end
