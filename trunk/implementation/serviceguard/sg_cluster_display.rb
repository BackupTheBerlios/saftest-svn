#!/usr/bin/ruby

$: << "%s/lib" % [ENV['SAFTEST_ROOT']]
$: << "%s/implementation/serviceguard" % [ENV['SAFTEST_ROOT']]
require 'SAFTestUtils'
require 'SGImplementation'

module SGImplementation

cluster = SGCluster.new
cluster.loadFromCommand()
#cluster.display()
print "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
print "<SAFCluster " + \
      " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " + \
      " xsi:noNamespaceSchemaLocation=\"SAFCluster.xsd\" " + \
      " schemaVersion=\"1\"> \n"
print "    <SAFNodeList>\n"

cluster.getNodes().each do |node|
    print "        <SAFNode>\n"
    print "            <id>%d</id>\n" % [node.getMapValue('id')]
    print "            <AddressList>\n"
    node.getInterfaces().each do |intf|
        print "                <Address>\n"
        print "                    <family>SA_CLM_AF_INET</family>\n"
        print "                    <length>4</length>\n"
        print "                    <value>%s</value>\n" % [intf.getIPAddress()]
        print "                </Address>\n"
    end
    print "            </AddressList>\n"
    print "            <name>%s</name>\n" % [node.getName()]
    memberStr = 'FALSE'
    if node.getMapValue('status') == 'up'
        memberStr = 'TRUE'
    end
    print "            <member>%s</member>\n" % [memberStr]
    print "            <bootTimestamp>%d</bootTimestamp>\n" % [node.getMapValue('boot_timestamp')]
    print "            <initialViewNumber>%d</initialViewNumber>\n" % [node.getMapValue('initial_incarnation')]
    print "        </SAFNode>\n"
end

print "    </SAFNodeList>\n"

print "    <SAFNodeChangeFlagList>\n"

cluster.getNodes().each do |node|
    print "        <SAFNodeChangeFlag>\n"
    print "            <id>%d</id>\n" % [node.getMapValue('id')]
    print "            <changeFlag></changeFlag>\n" 
    print "        </SAFNodeChangeFlag>\n"
end

print "    </SAFNodeChangeFlagList>\n"

print "</SAFCluster>\n"
end # module SGImplementation
