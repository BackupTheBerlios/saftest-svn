#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'test/unit'

clmDir = "%s/AIS-clm-%s" % \
         [ENV['SAFTEST_ROOT'], 
          SAFTestUtils::SAFTestUtils.getAISLibVersion()]
$: << clmDir

class SelectionObjectGetCase < Test::Unit::TestCase
    require 'CLMTestDriver'

    def test_run()
        CLMTestDriver::CLMTestDriver.getLongLivedDrivers(nil).each do |d|
            d.getAllTestResourceIDs().each do |r|
                d.selectObjectGet(r, false,
                                  SAFTestUtils::SAFTestUtils.SA_AIS_OK)
            end
        end
    end
end
