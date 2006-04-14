module SAFTestEngine

require 'xmlparser'
require 'SAFTestUtils'

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]

class SAFTestEngineCase < SAFTest::SAFTestUtils
    def initialize()
        super()
        @name = ''
        @cmd = ''
        @weight = 1
    end

    def name()
        return @name
    end

    def name=(name)
        @name = name
    end

    def command()
        return @cmd
    end

    def command=(cmd)
        @cmd = cmd
    end

    def weight()
        return @weight
    end

    def weight=(weight)
        @weight = weight
    end

    def run(node)
        fullCmd = "%s/cases/%s" % [ENV['SAFTEST_ROOT'], @cmd]
        exitCode = @safSys.runCommand(fullCmd, node) do |line|
            line.chomp!
            log(line)
        end
        return exitCode
    end

    def saveToXML(file)
        file.puts " <SAFTestCase>\n"
        file.puts "  <name>%s</name>\n" % [@name]
        file.puts "  <cmd>%s</cmd>\n" % [@cmd]
        file.puts "  <weight>%d</weight>" % [@weight]
        file.puts " </SAFTestCase>\n"
    end
end

class SAFTestBundle
    # cases is a hash from the spec name to a hash of cases.
    # For the top-level cases, we use spec as 'MAIN'
    def initialize(xmlPath)
        @cases = {}
        @specCases = {}
        @mainMode = nil # sequential or random
        loadFromXML(xmlPath)
    end

    def getMainMode()
        return @mainMode
    end

    def cases()
        return @cases
    end

    def addSpecCases(specName, cases)
        @specCases[specName] = cases 
    end
    
    def loadFromXML(xmlPath)
        f = open(File.expand_path(xmlPath), 'r')
        xmlLines = f.readlines()
        f.close
        xml = ''
        xmlLines.each do |line|
            if line =~ /^<\?xml.*/ then
                next
            end
            xml += line
        end

        currentCase = nil
        currentType = nil
        currentMode = nil
        currentElement = nil
        parser = XML::Parser.new
        begin
            parser.parse(xml) do |type, name, data|
                case type
                    when XML::Parser::START_ELEM
                        case name
                            when ''
                                raise 'Empty Start Tag?'
                            when 'SAFTestCaseList'
                                currentType = data['type']
                                currentMode = data['mode']
                                if currentType == 'main'
                                    if currentMode != 'sequential' and
                                       currentMode != 'random'
                                        raise "Invalid mode #{currentMode}"
                                    end
                                    @mainMode = currentMode
                                end
                                @cases[currentType] = []
                            when 'SAFTestCase'
                                # No attributes on these tags
                                currentCase = SAFTestEngineCase.new
                                currentElement = name
                            else
                                currentElement = name
                        end

                    when XML::Parser::CDATA
                        case currentElement
                            when 'name'
                                currentCase.name = data
                            when 'cmd'
                                currentCase.command = data
                            when 'weight'
                                currentCase.weight = data.to_i
                        end

                    when XML::Parser::END_ELEM
                        case name
                            when 'SAFTestCase'
                                @cases[currentType] << currentCase
                                currentCase = nil
                        end
                        currentElement = nil
                end
            end
        rescue XMLParserError
            line = parser.line
            raise "XML parse error #{$!}: LINE #{line} FROM #{xml}\n"
        end
    end       

    # This function is used to merge the top-level cases file with
    # the per-spec cases file.  There is a certain order to things, which is
    # detailed below.  Note that the order of each spec doesn't matter, because 
    # it doesn't matter if we run CLM test cases before LCK test cases or
    # vice-versa; we just want to make sure that within each spec the order
    # is preserved so that sequential test cases work properly.
    #
    # Initial cases
    #   1.  top-level initial cases
    #   2.  per-spec initial cases
    #
    # Main cases
    #   1.  top-level main cases
    #   2.  per-spec main cases
    #   
    # Final cases
    #   1.  per-spec final cases
    #   2.  top-level final cases
    #
    # Yes, the order is reversed for initial and final cases.  That is 
    # deliberate.

    def writeToXML(fileName)
        f = File.new(fileName, "w")
        f.puts "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        f.puts "<SAFTestBundle " + \
              " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " + \
              " xsi:noNamespaceSchemaLocation=\"SAFTestBundle.xsd\" " + \
              " schemaVersion=\"1\">\n"

        # Initial Cases
        f.puts "<SAFTestCaseList type=\"initial\" mode=\"sequential\">\n"
        @cases['initial'].each do |c|
            c.saveToXML(f)
        end

        @specCases.keys().each do |specName|
            f.puts "<!-- Initial Cases for spec %s -->\n" % [specName.upcase()]
            @specCases[specName]['initial'].each do |c|
                c.saveToXML(f)
            end
        end
        f.puts "</SAFTestCaseList>\n"
        
        # Main Cases
        f.puts "<SAFTestCaseList type=\"main\" mode=\"%s\">\n" % [@mainMode]
        @cases['main'].each do |c|
            c.saveToXML(f)
        end

        @specCases.keys().each do |specName|
            f.puts "<!-- Main Cases for spec %s -->\n" % [specName.upcase()]
            @specCases[specName]['main'].each do |c|
                c.saveToXML(f)
            end
        end
        f.puts "</SAFTestCaseList>\n"

        # Final Cases
        f.puts "<SAFTestCaseList type=\"final\" mode=\"sequential\">\n"
        @specCases.keys().each do |specName|
            f.puts "<!-- Final Cases for spec %s -->\n" % [specName.upcase()]
            @specCases[specName]['final'].each do |c|
                c.saveToXML(f)
            end
        end

        @cases['final'].each do |c|
            c.saveToXML(f)
        end

        f.puts "</SAFTestCaseList>\n"
        
        f.puts "</SAFTestBundle>\n"
    end
end # class

end # module
