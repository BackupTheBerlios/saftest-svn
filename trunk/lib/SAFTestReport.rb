module SAFTest

require 'SAFTestUtils'

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]

class SAFTestCaseResult
    def initialize()
        @passedCount = 0
        @failedCount = 0
        @skippedCount = 0
        @notConfiguredCount = 0
    end

    def addPassed()
        @passedCount += 1
    end

    def addFailed()
        @failedCount += 1
    end

    def addSkipped()
        @skippedCount += 1
    end

    def addNotConfigured()
        @notConfiguredCount += 1
    end

    def getPassedCount()
        return @passedCount
    end

    def getFailedCount()
        return @failedCount
    end

    def getSkippedCount()
        return @skippedCount
    end

    def getNotConfiguredCount()
        return @notConfiguredCount
    end

    def numInstances()
        return (getPassedCount() + getFailedCount() + getSkippedCount()
                + getNotConfiguredCount())
    end
end

class SAFTestReport < SAFTestUtils
    def initialize()
        super()
        @config = SAFTest::SAFTestConfig.new()
        @config.loadFromXML(configXMLFile())
        @pidfile = "%s/engine.pid" % [runDir()]
        @logfile = "%s/%s.log" % [logDir(), 
                                 @config.getStrValue("main", "testType")]
        @caseResults = {} # caseName => SAFTestCaseResult object
    end

    def runTestCase(testCase, testPhase, node)
        nodeLog = ""
        if node != nil
            nodeLog = " on #{node}"
        end

        log("BEGIN #{testPhase} #{testCase.name}#{nodeLog}")

        startTime = Time.now()
        exitCode = testCase.run(node)
        endTime = Time.now()

        elapsedTime = ((endTime - startTime).to_f).to_s
        log("END #{testPhase} #{testCase.name}#{nodeLog} " + 
            "status #{exitCode} in #{elapsedTime} seconds\n")
        return exitCode
    end

    def insertCaseResults(caseName)
        if not @caseResults.has_key?(caseName)
            @caseResults[caseName] = SAFTestCaseResult.new
        end
    end

    def printReport()
        name = nil
        status = nil
        caseLines = nil
        failedLines = []

        IO.foreach(@logfile) do |line|
            # Dec 20 21:49:13 BEGIN main node_get_local
            if line =~ /^(\S+) (\d+) (\S+) BEGIN (\S+) (\S+) (.*)/
                name = $5
                status = nil
                caseLines = [line]
                insertCaseResults(name)
                next
            # Dec 20 21:49:13 END main node_get_local status 1 FAILED in 0.03 seconds
            elsif line =~ /^(\S+) (\d+) (\S+) END (\S+) (\S+) status (\d+) (\S+) .*/
                caseLines << line
                endName = $5
                status = $7
                if endName != name
                    raise "Error: name is #{name} but endName is #{endName}"
                end
                
                if status == "PASSED"
                    @caseResults[name].addPassed()
                elsif status == "SKIPPED"
                    @caseResults[name].addSkipped()
                elsif status == "NOT_CONFIGURED"
                    @caseResults[name].addNotConfigured()
                else
                    @caseResults[name].addFailed()
                    failedLines.concat(caseLines)
                    failedLines << "\n" 
                end

                name = nil
                caseLines = nil
                next
            else
                if name != nil            
                    caseLines << line
                end
            end
        end

        print "Aggregate Results:\n\n"
        print "Name Passed Failed Skipped NotConfigured\n"
        @caseResults.keys.each do |caseName|
            results = @caseResults[caseName]
            print "%s %d %d %d %d\n" % [caseName, 
                                        @caseResults[caseName].getPassedCount(),
                                        @caseResults[caseName].getFailedCount(), 
                                        @caseResults[caseName].getSkippedCount(),
                                        @caseResults[caseName].getNotConfiguredCount()]
        end

        print "\n"
        print "Failed Test Case Logs:\n\n"
        failedLines.each do |line|
            print line
        end
    end

    def runCases()
        @state = @@RUNNING_INITIAL_CASES
        @cases['initial'].each do |testCase|
            exitCode = runTestCase(testCase, 'initial', nil)
        end

        @state = @@RUNNING_MAIN_CASES
        if @bundle.getMainMode() == 'sequential' then
            @cases['main'].each do |testCase|
                exitCode = runTestCase(testCase, 'main', nil)
            end
        else
            if @bundle.getMainMode() != 'random' then
                raise "mainMode must be 'sequential' or 'random', but is %s" \
                      % [@bundle.getMainMode()]
            end

            while (not @needToHalt)
                testCase = selectRandomMainTestCase()
                exitCode = runTestCase(testCase, 'main', nil)
            end
        end

        @state = @@RUNNING_FINAL_CASES
        @cases['final'].each do |testCase|
            exitCode = runTestCase(testCase, 'final', nil)
        end

        @testEndTime = Time.now()
        elapsedTime = (@testEndTime - @testStartTime).to_i
        log("FINISHED in " + formatSeconds(elapsedTime))
    end

end # class

end # module
