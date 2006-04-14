module SAFTest

require 'SAFTestUtils'
require 'xmlparser'

class SAFNetworkAddressImplementation < SAFTestUtils
    def initialize()
        super()
        @family = 'SA_CLM_AF_INET'
        @length = 4
        @value = '0.0.0.0'
    end

    def getFamily()
        return @family
    end

    def setFamily(family)
        @family = family
    end

    def getLength()
        return @length
    end

    def setLength(length)
        @length = length
    end

    def getValue()
        return @value
    end

    def setValue(value)
        @value = value
    end

    def ==(other)
        return false unless SAFNetworkAddressImplementation === other
        return false unless @family == other.getFamily()
        return false unless @length == other.getLength()
        return false unless @value == other.getValue()
        return true
    end
    alias eql? ==

    def display()
        print "        Address Family: %s\n" % [getFamily()]
        print "        Address Length: %s\n" % [getLength()]
        print "        Address Value: %s\n" % [getValue()]
    end
end

class SAFNodeImplementation < SAFTestUtils
    def initialize()
        super()
        @name = ''
        @id = 0
        @addresses = []
        @member = false
        @bootTimestamp = 0
        @initialViewNumber = 0
    end

    def getName()
        return @name
    end

    def setName(name)
        @name = name
    end

    def getID()
        return @id
    end

    def setID(id)
        @id = id
    end

    def getMember()
        return @member
    end

    def setMember(member)
        @member = member
    end

    def getBootTimestamp()
        return @bootTimestamp
    end

    def setBootTimestamp(bootTimestamp)
        @bootTimestamp = bootTimestamp
    end

    def getInitialViewNumber()
        return @initialViewNumber
    end

    def setInitialViewNumber(initialViewNumber)
        @initialViewNumber = initialViewNumber
    end

    def getAddresses()
        return @addresses
    end

    def addAddress(addr)
        @addresses << addr
    end

    def loadFromXML(xml)
        currentAddress = nil
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
                            when 'id'
                                setID(data.to_i)
                            when 'name'
                                setName(data)
                            when 'member'
                                case data
                                    when 'TRUE'
                                        setMember(true)
                                    when 'FALSE'
                                        setMember(false)
                                    else
                                        raise "Unknown member value %s" % \
                                               [value]
                                end
                            when 'bootTimestamp'
                                setBootTimestamp(data.to_i)
                            when 'initialViewNumber'
                                setInitialViewNumber(data.to_i)
                            when 'Address'
                                currentAddress = 
                                    SAFNetworkAddressImplementation.new()
                            when 'family'
                                currentAddress.setFamily(data)
                            when 'length'
                                currentAddress.setLength(data)
                            when 'value'
                                currentAddress.setValue(data)
                        end

                    when XML::Parser::END_ELEM
                        case name
                            when 'Address'
                                addAddress(currentAddress)
                                currentAddress = nil
                        end
                        currentElement = nil
                end
            end
        rescue XMLParserError
            line = parser.line
            raise "XML parse error #{$!}: LINE #{line} FROM #{xml}\n"
        end

    end

    def isLocalNode()
        return getName() == fullHostname() || getName() == simpleHostname()
    end

    def ==(other)
        return false unless SAFNodeImplementation === other
        return false unless @name == other.getName()
        return false unless @id == other.getID()
        return false unless @member == other.getMember()
        return false unless @bootTimestamp == other.getBootTimestamp()
        return false unless @initialViewNumber == other.getInitialViewNumber()
        
        # It doesn't need to be the case that all addresses are exist on both
        # copies of the node.  Just that they have at least one shared address
        @addresses.each do |selfAddr|
            other.getAddresses().each do |otherAddr|
                return true if selfAddr == otherAddr
            end
        end
        return false
    end
    alias eql? ==

    def display()
        print "Node:\n"
        print "    Name: %s\n" % [getName()]
        print "    Node ID: %d\n" % [getID()]
        print "    Member: %s\n" % [getMember().to_s]
        print "    Boot Timestamp: %d\n" % [getBootTimestamp()]
        print "    Initial View Number: %d\n" % [getInitialViewNumber()]
        ndx = 0
        getAddresses().each do |addr|
            ndx += 1
            print "    Address %d" % [ndx]
            addr.display()
        end
    end
end

class SAFClusterImplementation < SAFTestUtils
    def initialize()
        super()
        @nodes = [] # Array of node objects

        @config = SAFTestConfig.new()
        @config.loadFromXML(configXMLFile())
    end

    def loadFromXML(xml)
        currentNode = nil
        currentAddress = nil
        currentElement = nil
        parser = XML::Parser.new
        begin
            parser.parse(xml) do |type, name, data|
                case type
                    when XML::Parser::START_ELEM
                        case name
                            when ''
                                raise 'Empty Start Tag?'
                            when 'SAFNode'
                                currentNode = SAFNodeImplementation.new()
                                # No attributes on these tags
                                #data.each do |key, value|
                                #end
                            else
                                currentElement = name
                        end

                    when XML::Parser::CDATA
                        case currentElement
                            when 'id'
                                currentNode.setID(data.to_i)
                            when 'name'
                                currentNode.setName(data)
                            when 'member'
                                case data
                                    when 'TRUE'
                                        currentNode.setMember(true)
                                    when 'FALSE'
                                        currentNode.setMember(false)
                                    else
                                        raise "Unknown member value %s" % \
                                               [value]
                                end
                            when 'bootTimestamp'
                                currentNode.setBootTimestamp(data.to_i)
                            when 'initialViewNumber'
                                currentNode.setInitialViewNumber(data.to_i)
                            when 'Address'
                                currentAddress = 
                                    SAFNetworkAddressImplementation.new()
                            when 'family'
                                currentAddress.setFamily(data)
                            when 'length'
                                currentAddress.setLength(data)
                            when 'value'
                                currentAddress.setValue(data)
                        end

                    when XML::Parser::END_ELEM
                        case name
                            when 'SAFNode'
                                @nodes << currentNode
                                currentNode = nil
                            when 'Address'
                                currentNode.addAddress(currentAddress)
                                currentAddress = nil
                        end
                        currentElement = nil
                end
            end
        rescue XMLParserError
            line = parser.line
            raise "XML parse error #{$!}: LINE #{line} FROM #{xml}\n"
        end

    end
    def addToMap(key, value)
        @map[key] = value
    end

    def getMapValue(key)
        return @map[key]
    end

    def getNodes()
        return @nodes
    end

    def getDownNodes()
        downNodes = []
        @nodes.each do |node|
            if !node.getMember()
                downNodes << node
            end
        end
        return downNodes
    end

    def getUpNodes()
        upNodes = []
        @nodes.each do |node|
            if node.getMember()
                upNodes << node
            end
        end
        return upNodes
    end

    def getLongLivedNodes()
        longLivedNodes = []
        shortLivedNodeNames = @config.getStrListValue('main',
                                                      'testShortLivedNodes')
        @nodes.each do |node|
            if not shortLivedNodeNames.include?(node.getName())
                longLivedNodes << node
            end
        end
        return longLivedNodes    
    end

    def getShortLivedNodes()
        shortLivedNodes = []
        shortLivedNodeNames = @config.getStrListValue('main',
                                                      'testShortLivedNodes')
        @nodes.each do |node|
            if shortLivedNodeNames.include?(node.getName())
                shortLivedNodes << node
            end
        end
        return shortLivedNodes    
    end

    def getNodeByName(nodeName)
        @nodes.each do |node|
            if node.getName == nodeName
                return node
            end
        end
        return nil
    end

    def getLocalNode()
        @nodes.each do |node|
            if node.isLocalNode()
                return node
            end
        end
        raise 'Should be able to find a local node'
    end

    def getRandomNode()
        return @nodes[rand(@nodes.length())]
    end

    def ==(other)
        return false unless SAFClusterImplementation === other
        return false unless other.getNodes().length == @nodes.length

        @nodes.each do |selfNode|
            otherNode = other.getNodeByName(selfNode.getName())
            return false unless otherNode != nil
            return false unless otherNode == selfNode
        end
        return true
    end
    alias eql? ==

    def display()
        getNodes().each do |node|
            node.display()
        end
    end

end

class SAFImplementation < SAFTestUtils
    def initialize()
        super()

        @displayClusterCommand = nil
        @startNodeCommand = nil
        @stopNodeCommand = nil
        @addNodeCommand = nil
        @deleteNodeCommand = nil
        @driverLogEnvironmentVariable = nil
        @driverEnvVars = {}

        @config = SAFTestConfig.new()
        @config.loadFromXML(configXMLFile())

        loadFromXML(implementationConfigFile())
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

        currentElement = nil
        currentName = nil

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
                            when 'DisplayClusterCommand'
                                @displayClusterCommand = data
                            when 'StartNodeCommand'
                                @startNodeCommand = data
                            when 'StopNodeCommand'
                                @stopNodeCommand = data
                            when 'AddNodeCommand'
                                @addNodeCommand = data
                            when 'DeleteNodeCommand'
                                @deleteNodeCommand = data
                            when 'DriverLogEnvironmentVariable'
                                @driverLogEnvironmentVariable = data
                            when 'name'
                                currentName = data
                            when 'value'
                                @driverEnvVars[currentName] = data
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

    def getDriverLogEnvironmentVariable()
        return @driverLogEnvironmentVariable
    end

    def getDriverEnvironmentVariables()
        return @driverEnvVars
    end

    def getCluster()
        cluster = SAFClusterImplementation.new()
        xml = ''
        array = captureCommand(@displayClusterCommand)
        ret = array[0]
        lines = array[1]
        inXml = false
        lines.each do |line|
            if not inXml
                # Filter out debugging stuff before the XML
                if line =~ /^<\?xml.*/ then
                    inXml = true
                else
                    next
                end
            end
            xml += line
        end

        cluster.loadFromXML(xml)
        return cluster
    end

    def getClusterFromFile(path)
        cluster = SAFClusterImplementation.new()
        f = open(File.expand_path(path), 'r')
        xmlLines = f.readlines()
        f.close
        xml = ''
        xmlLines.each do |line|
            xml += line
        end

        cluster.loadFromXML(xml)
        return cluster
    end

    def getNodeFromFile(path)
        node = SAFNodeImplementation.new()
        f = open(File.expand_path(path), 'r')
        xmlLines = f.readlines()
        f.close
        xml = ''
        xmlLines.each do |line|
            xml += line
        end

        node.loadFromXML(xml)
        return node
    end

    def stopNode(nodeName)
        stopCommand = "%s %s" % [@stopNodeCommand, nodeName]
        runAndCheckCommand(stopCommand, 
                           EXPECT_SUCCESS,
                           "Unable to halt node %s" % [nodeName])
    end

    def startNode(nodeName)
        startCommand = "%s %s" % [@startNodeCommand, nodeName]
        runAndCheckCommand(startCommand, 
                           EXPECT_SUCCESS,
                           "Unable to start node %s" % [nodeName])
    end

    def addNode(nodeName)
        addCommand = "%s %s" % [@addNodeCommand, nodeName]
        runAndCheckCommand(addCommand, 
                           EXPECT_SUCCESS,
                           "Unable to add node %s" % [nodeName])
    end

    def deleteNode(nodeName)
        deleteCommand = "%s %s" % [@deleteNodeCommand, nodeName]
        runAndCheckCommand(deleteCommand, 
                           EXPECT_SUCCESS,
                           "Unable to delete node %s" % [nodeName])
    end

    def ensureAllNodesAreUp(cluster)
        cluster.getDownNodes().each do |node|
            startNode(node.getName())
        end
    end

end # class SAFImplementation

end # module SAFTest
