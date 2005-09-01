module SGImplementation

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]

require 'SAFTestUtils'

class SGBase < SAFTestUtils::SAFTestUtils
    def initialize()
        super()
        @map = {} # key=>value
    end

    def addToMap(key, value)
        @map[key] = value
    end

    def getMapValue(key)
        return @map[key]
    end

    def processLineToMap(line)
        if line =~ /^([A-z_]+)=(.*)/ then
            key, value = $1, $2
            addToMap(key, value)
        end
    end
end

class SGNetworkInterface < SGBase
    def initialize()
        super()
    end

    def getName()
        return @map['name']
    end

    def getIPAddress()
        return @map['ip_address']
    end

    def processLine(line)
        if line =~ /^ip_address:(.+)\|(.*)/ then
            ip_address, line = $1, $2
            @map['ip_address'] = $1
        else
            processLineToMap(line)
        end
    end

    def display()
        print "    Interface: %s\n" % [@map['name']]
        print "        IP Address: %s\n" % [@map['ip_address']]
    end
end

class SGNode < SGBase
    def initialize()
        super()
        @interfaces = {} # name=>interface object
    end

    def processLine(line)
        if line =~ /^interface:([A-z0-9]+)\|(.*)/ then
            interfaceName, line = $1, $2
            if @interfaces.has_key?(interfaceName)
                interface = @interfaces[interfaceName]
            else
                interface = SGNetworkInterface.new()
                @interfaces[interfaceName] = interface
            end
            interface.processLine(line)
        else
            processLineToMap(line)
        end
    end

    def getName()
        return @map['name']
    end

    def getInterfaces()
        interfaces = []
        @interfaces.keys().each do |interfaceName|
            interfaces << @interfaces[interfaceName]
        end
        return interfaces
    end

    def display()
        print "Node: %s\n" % [@map['name']]
        getInterfaces().each do |intf|
            intf.display()
        end
    end
end

class SGCluster < SGBase
    def initialize()
        super()
        @nodes = {} # name=>nodes object
    end

    def loadFromCommand()
        array = captureCommand("cmviewcl -v -f line")
        ret = array[0]
        lines = array[1]
        lines.each do |line|
            if line =~ /^node:([A-z0-9]+)\|(.*)/ then
                nodeName, line = $1, $2
                if @nodes.has_key?(nodeName)
                    node = @nodes[nodeName]
                else
                    node = SGNode.new()
                    @nodes[nodeName] = node
                end
                node.processLine(line)
            else
                processLineToMap(line)
            end
        end
    end

    def getNodes()
        nodes = []
        @nodes.keys().each do |nodeName|
            nodes << @nodes[nodeName]
        end
        return nodes
    end

    def getNodeByName(nodeName)
        return @nodes[nodeName]
    end

    def display()
        print "Cluster Name: %s\n" % [@map['name']]
        print "Incarnation: %s\n" % [@map['incarnation']]
        getNodes().each do |node|
            node.display()
        end
    end

end

end # module SGImplementation
