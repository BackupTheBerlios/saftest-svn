module SAFTest

require 'SAFTestUtils'
require 'rexml/document'

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
        doc = REXML::Document.new(xml)

        doc.elements.each("SAFNode") { |element|
            element.each_element_with_text { |ne|
                if ne.name == "id"
                    setID(ne.get_text.to_s.to_i)
                elsif ne.name == "AddressList"
                    ne.each_element_with_text { |addressElement|
                        address = SAFNetworkAddressImplementation.new()
                        addressElement.each_element_with_text { |ae|
                            text = ae.get_text.to_s
                            if ae.name == "family"
                                address.setFamily(text)
                            elsif ae.name == "length"
                                address.setLength(text)
                            elsif ae.name == "value"
                                address.setValue(text)
                            else
                                raise "Unknown address element %s" % [text]
                            end
                        }
                        addAddress(address)
                    }
                elsif ne.name == "name"
                    setName(ne.get_text.to_s)
                elsif ne.name == "member"
                    if ne.get_text.to_s == "TRUE"
                        setMember(true)
                    elsif ne.get_text.to_s == "FALSE"
                        setMember(false)
                    else
                        raise "Unknown member element %s" % [ne.get_text.to_s]
                    end
                elsif ne.name == "bootTimestamp"
                    setBootTimestamp(ne.get_text.to_s.to_i)
                elsif ne.name == "initialViewNumber"
                    setInitialViewNumber(ne.get_text.to_s.to_i)
                else
                    raise "Unknown element %s" % [ne.name]
                end
            }
        }
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
        doc = REXML::Document.new(xml)

        doc.elements.each("SAFCluster/SAFNodeList/SAFNode") { |element|
            node = SAFNodeImplementation.new()
            node.loadFromXML(element.to_s)
            @nodes << node
        }
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
        file = open(File.expand_path(xmlConfigFile), 'r')
        doc = REXML::Document.new(file)

        doc.elements.each("SAFImplementation") { |element|
            element.each_element_with_text { |e|
                text = e.get_text.to_s
                if e.name == "DisplayClusterCommand"
                    @displayClusterCommand = text
                elsif e.name =="StartNodeCommand"
                    @startNodeCommand = text
                elsif e.name =="StopNodeCommand"
                    @stopNodeCommand = text
                elsif e.name =="AddNodeCommand"
                    @addNodeCommand = text
                elsif e.name =="DeleteNodeCommand"
                    @deleteNodeCommand = text
                elsif e.name =="DriverLogEnvironmentVariable"
                    @driverLogEnvironmentVariable = text
                elsif e.name =="DriverEnvironmentVariableList"
                    # Do nothing, we'll handle this later
                else
                    raise "Unknown implementation element %s" % [e.name]
                end
            }
        }

        doc.elements.each("SAFImplementation/DriverEnvironmentVariableList/DriverEnvironmentVariableEntry") { |element|
            name = nil
            value = nil
            element.each_element_with_text { |e|
                if e.name == "name"
                    name = e.get_text.to_s
                elsif e.name == "value"
                    value = e.get_text.to_s
                else
                    raise "Unknown environment variable element %s" % [e.name]
                end
            }
            @driverEnvVars[name] = value
        }
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
