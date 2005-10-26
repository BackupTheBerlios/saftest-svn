module SAFTest

class SAFTestConfig < SAFTestUtils
    def initialize(verbose = false)
        super()
        # Hash from Section Name to Config Hash
        @config = {}
    end

    def getStrValue(section, key)
        return @config[section][key]
    end

    def getIntValue(section, key)
        return @config[section][key].to_i
    end

    def insertConfigValue(section, key, value)
        if not @config.has_key?(section)
            @config[section] = {}
        end
        @config[section][key] = value
    end

    def promptArray(section, key, label, optionArray, defaultOption)
        optionString = ''
        optionArray.each do |option|
            optionString = '%s|%s' % [optionString, option]
        end
        while(true)
            print "%s (%s) [%s]: " % [label,
                                      optionString[1, optionString.length],
                                      defaultOption]
            answer = gets
            answer.chomp!
            if answer == ''
                answer = defaultOption
            end

            if optionArray.include?(answer)
                insertConfigValue(section, key, answer)
                break
            else
                print "Invalid answer: \"%s\"\n" % [answer]
            end
        end
    end

    def promptYesNo(section, key, label, defaultIsYes)
        while(true)
            if defaultIsYes
                defaultOption = 'yes'
            else
                defaultOption = 'no'
            end

            print "%s (yes/no) [%s]: " % [label, defaultOption]
            answer = gets
            answer.chomp!
            if answer == ''
                answer = defaultOption
            end

            if answer.downcase()[0] == 'y'[0]
                insertConfigValue(section, key, 'yes')
                break
            elsif answer.downcase()[0] == 'n'[0]
                insertConfigValue(section, key, 'no')
                break
            else
                print "Invalid answer: \"%s\"\n" % [answer]
            end
        end
    end

    def promptInt(section, key, label, min, max, defaultOption)
        while(true)
            print "%s (%d-%d) [%d]: " % [label, min, max, defaultOption]
            answer = gets
            answer.chomp!
            if answer == ''
                answer = defaultOption
            else
                answer = answer.to_i
            end

            if answer >= min and answer <= max
                insertConfigValue(section, key, answer.to_s)
                break
            else
                print "Invalid answer: \"%d\"\n" % [answer]
            end
        end
    end

    def save()
        f = File.new(configXMLFile(), "w")
        f.puts "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        f.puts "<SAFTestConfig " + \
              " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " + \
              " xsi:noNamespaceSchemaLocation=\"SAFTestConfig.xsd\" " + \
              " schemaVersion=\"1\">\n"

        f.puts "<SAFTestConfigSectionList>\n"
        @config.keys().each do |section|
            f.puts " <SAFTestConfigSection>\n"
            f.puts "  <SAFTestConfigSectionName>%s</SAFTestConfigSectionName>\n" % [section]
            f.puts "  <SAFTestConfigEntryList>\n"
            @config[section].keys().each do |key|
                f.puts "   <SAFTestConfigEntry>\n"
                f.puts "    <name>%s</name>\n" % [key]
                f.puts "    <value>%s</value>\n" % [@config[section][key]]
                f.puts "   </SAFTestConfigEntry>\n"
            end
            f.puts "  </SAFTestConfigEntryList>\n"
            f.puts " </SAFTestConfigSection>\n"
        end
        f.puts "</SAFTestConfigSectionList>\n"
        f.puts "</SAFTestConfig>\n"
    end

    def loadFromXML(xmlConfigFile)
        f = open(File.expand_path(xmlConfigFile), 'r')
        xmlLines = f.readlines()
        f.close
        xml = ''
        xmlLines.each do |line|
            if line =~ /^<\?xml.*/ then
                next
            end
            xml += line
        end

        currentSection = nil
        currentName = nil
        currentElement = nil

        parser = XML::Parser.new
        begin
            parser.parse(xml) do |type, name, data|
                case type
                    when XML::Parser::START_ELEM
                        case name
                            when ''
                                raise 'Empty Start Tag?'
                            else
                                currentElement = name
                        end
                    when XML::Parser::CDATA
                        case currentElement
                            when 'SAFTestConfigSectionName'
                                currentSection = data
                            when 'name'
                                currentName = data
                            when 'value'
                                print "Adding (%s, %s, %s)\n" % \
                                    [currentSection, currentName, data]
                                insertConfigValue(currentSection, currentName,
                                                  data)
                        end
                    when XML::Parser::END_ELEM
                        currentElement = nil
                end
            end
        rescue XMLParserError
            line = parser.line
            raise "XML parse error #{$!}: LINE #{line} FROM #{xml}\n"
        end
    end

end # class

end # module
