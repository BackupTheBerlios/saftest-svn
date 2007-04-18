#! /usr/bin/perl -w
=pod
The utility function for deleting node(s) to cluster.
It assumes that the cluster is down on nodes to be deleted from
the cluster. 
=cut

sub usage {
    print "usage: cmdelnode node1 node2 ...\n";
    exit 1;
}

#
# make sure at least one node is specified in the argument list
#
if (scalar(@ARGV) <=0) {
    usage();
}

#
# Create a list of nodes to be deleted from the cluster
#
my @nodeList = ();
my $i = 0;

foreach (@ARGV) {
     push(@nodeList, $ARGV[$i]);
     $i = $i+1;
}

my $j = 0;
my $asciiOld = "/tmp/asciiOld_$$";
my $asciiNew = "/tmp/asciiNew_$$";
my $cmd = "cmgetconf > $asciiOld";

system($cmd);

foreach (@nodeList) {

        my $node = $_;
        open (OLD, "< $asciiOld") or die "Failed to open $asciiOld";
        $j = $j+1;  
        open (NEW, "> $asciiNew") or die "failed to open $asciiNew";

        while (<OLD>) {
                 if(/^NODE_NAME.*$node/) {
                    s/NODE_NAME/\#NODE_NAME/;
                    print NEW "$_";
                    last; 
                }
                print NEW "$_";
        }

        while (<OLD>) {
                 if ((/^\s*NETWORK_INTERFACE/)|(/^\s*HEARTBEAT_IP/)|(/^\s*STATIONARY_IP/)|(/^\s*CLUSTER_LOCK_LUN/)) {
                     s/$_/\#$_/;
                     print NEW "$_";
                 }
                 else {
                     print NEW "$_";
                     last;
                 }
        }

        while (<OLD>) {
                 print NEW "$_";
        }

        close(OLD);
        close(NEW);

        $asciiOld = $asciiNew;
        $asciiNew = $asciiNew."_".$j;
}

system("cmapplyconf -f -v -C $asciiOld");
   
system("rm -f /tmp/*$$*");
