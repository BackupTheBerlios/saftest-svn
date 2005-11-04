#!/usr/bin/ruby

module SAFTest

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
require 'SAFTestCase'

clmDir = "%s/cases/clm" % [ENV['SAFTEST_ROOT']]
$: << clmDir
require 'CLMTestDriver'

class InitializeCase < SAFTestCase
    def initialize()
        super()
    end

    def run()
        @implementation.getCluster().getNodes().each do |node|
            CLMTestDriver.getLongLivedDrivers(node).each do |d|
                resourceID = d.createTestResource()
                d.init(resourceID, true, true, "SA_DISPATCH_ALL",
                       SAFTestUtils.SA_AIS_OK)
            end
        end
    end
end

test = InitializeCase.new()
test.run()

end # module
