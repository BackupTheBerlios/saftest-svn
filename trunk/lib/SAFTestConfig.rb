module SAFTest

require 'rexml/document'
require 'SAFTestUtils'

class SAFTestConfig < SAFTestUtils
    def initialize(verbose = false)
        super()
        # Hash from Section Name to Config Hash
        @config = {}
    end

    def valueExists?(section, key)
        if @config.has_key?(section)
            if @config[section].has_key?(key)
                return true
            end
        end
        
        return false
    end

    def getStrValue(section, key)
        return @config[section][key]
    end

    def getIntValue(section, key)
        return @config[section][key].to_i
    end

    def getStrListValue(section, key)
        return [].concat(@config[section][key].split(' '))
    end

    def getArrValue(section, key)
        return @config[section][key]
    end

    def insertConfigValue(section, key, value)
        if not @config.has_key?(section)
            @config[section] = {}
        end
        @config[section][key] = value
    end

    def valueIsYes(section, key)
        value = getStrValue(section, key)
        if value == 'yes'
            return true
        else
            if value != 'no'
                error = "Expected 'yes' or 'no' for section=#{section} " +
                        "key=#{key}, but got '#{value}'"
                raise error
            end
        end
        return false
    end

    def promptList(section, key, label, defaultList, allowEmpty)
        return if valueExists?(section, key)
        defaultString = ''
        isFirst = true
        defaultList.each do |option|
            if isFirst == true
                defaultString = '%s' % [option]
                isFirst = false
            else
                defaultString = '%s %s' % [defaultString, option]
            end
        end
        while(true)
            print "%s [%s]: " % [label, defaultString]
            answer = gets
            answer.chomp!
            if answer == ''
                if defaultList == nil
                    answer = ''
                else
                    answer = defaultString
                end
            end

            if answer != '' or allowEmpty == true
                insertConfigValue(section, key, answer)
                break
            else
                print "Invalid answer: \"%s\"\n" % [answer]
            end
        end
    end

    def promptArray(section, key, label, optionArray, defaultOption)
        return if valueExists?(section, key)
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
        return if valueExists?(section, key)
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
        return if valueExists?(section, key)
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

    def promptStr(section, key, label, defaultOption)
        return if valueExists?(section, key)
        while(true)
            print "%s [%s]: " % [label, defaultOption]
            answer = gets
            answer.chomp!
            if answer == ''
                answer = defaultOption
            else
                answer = answer
            end

            if (not iterator?) or (yield answer)
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
        file = open(File.expand_path(xmlConfigFile), 'r')
        doc = REXML::Document.new(file)

        doc.elements.each("SAFTestConfig/SAFTestConfigSectionList/SAFTestConfigSection") { 
            |sectionElement|
            sectionName = nil
            sectionElement.each_element_with_text { |e|
                if e.name == "SAFTestConfigSectionName"
                    sectionName = e.get_text.to_s
                elsif e.name == "SAFTestConfigEntryList"
                    e.each_element_with_text { |configEntryListElement|
                        entryName = nil
                        entryValue = nil
                        configEntryListElement.each_element_with_text {
                            |configEntryElement|
                            elementName = configEntryElement.name
                            if elementName == "name"
                                entryName = configEntryElement.get_text.to_s
                            elsif elementName == "value"
                                entryValue = configEntryElement.get_text.to_s
                            else
                                raise "Unknown element %s" % [elementName]
                            end    
                        }
                        insertConfigValue(sectionName, entryName, entryValue)
                    }
                else
                    raise "Unknown element %s" % [e.name]
                end
            }
        }
    end

end # class

end # module
