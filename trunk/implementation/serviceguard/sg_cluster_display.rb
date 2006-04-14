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
    print "            <bootTimestamp>0</bootTimestamp>\n"
    print "            <initialViewNumber>0</initialViewNumber>\n"
    print "        </SAFNode>\n"
end

print "    </SAFNodeList>\n"
print "</SAFCluster>\n"
end # module SGImplementation
