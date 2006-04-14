#!/usr/bin/ruby

module SAFTest

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestCase'

clmDir = "%s/cases/clm" % [ENV['SAFTEST_ROOT']]
$: << clmDir
require 'CLMTestDriver'

class SelectionObjectGetCase < SAFTestCase
    def initialize()
        super()
    end

    def run()
        @implementation.getCluster().getNodes().each do |node|
            CLMTestDriver.getLongLivedDrivers(node).each do |d|
                d.getAllTestResources().each do |r|
                    d.selectObjectGet(r, false, SAFTestUtils.SA_AIS_OK)
                end
            end
        end
        passed()
    end
end

test = SelectionObjectGetCase.new()
test.run()

end # module
