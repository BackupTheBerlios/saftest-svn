#! /usr/bin/perl -w
=pod
utility function for adding node(s) to cluster
=cut

sub usage {
    print "usage: cmaddnode node1 node2 ...\n";
    exit 1;
}

sub captureCommand {
    my ($cmd) = @_;
    my $tmp = "/tmp/cmaddnode_$$.out";
    system("$cmd > $tmp");
    open (FH, "< $tmp") or die "Failed to open $tmp";
    my @lines = ();
    while (<FH>) {
        chomp($_);
        push(@lines,$_);
    }
    close(FH);
    system("rm -f $tmp");
    return @lines;
}
#
# make sure at least one node is specified in the arg list.
#
if (scalar(@ARGV) <=0) {
    usage();
}

#
# Create a list of nodes to be added to this cluster.
#
my @nodeList = ();
my $i = 0;

foreach (@ARGV) {
     push(@nodeList, $ARGV[$i]);
     $i = $i+1;
}

#
# Get the existing cluster name and member node names.
#
my $cmd = "cmviewcl -f line | grep name";
my @output = captureCommand($cmd);

my $clusterLine = shift(@output);
my @cluster = split ('=',$clusterLine);
my $clusterName = $cluster[1];
my @nodes;
my @tmp1;

foreach my $line (@output) {
     @tmp1 = split("=", $line);
     push(@nodes, $tmp1[1]);
}

#
# The @nodes array contains the existing node names.
# Append the additional nodes to be added to this cluster.
#
push(@nodes, @nodeList);

$cmd = "cmviewcl -v -f line | grep cluster_lock | grep lun";
@output = captureCommand($cmd);
my $asciiFile = "/tmp/tmp_$$.ascii";
$cmd = "cmquerycl -v -c $clusterName -C $asciiFile ";

if (@output) {
    @tmp1 = split ('=',$output[0]);
    my $lockLun = $tmp1[1];
    $cmd = $cmd . join (' ', map{"-n $_ -L $lockLun"} @nodes);
} else {
    $cmd = $cmd . join (' ', map{"-n $_"} @nodes);
}
print "$cmd \n";
system($cmd);
system("cmapplyconf -f -v -C $asciiFile");
system("rm -f $asciiFile");
