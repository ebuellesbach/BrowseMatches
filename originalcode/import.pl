#!/usr/bin/perl -w

# import.pl
# Benedict Brown
#
# simple script to convert a matches XML file to a SQL script for importing into a database
#
# argument 0 is the match XML file
# argument 1 is the user
# argument 2 is the matcher

# in this version the attribute IDs are hard-coded to make life easier

# XML parser
use XML::Simple;

# attribute mapping
%attr = (
    "error" => 1,
    "old_volume" => 2,
    "volume" => 3,
    "overlap" => 4,
    "Probability" => 5
    # "xf" => 6
);

@statuses = ( -1, 1, .5, 0, -1 );

# get uid
$user = $ARGV[1];
print "SELECT fnUser('$user') INTO \@uid;\n";

# get matcher id
$matcher = $ARGV[2];
print "SELECT fnMatcher('$matcher') INTO \@mid;\n\n";

# open and parse the XML file
$xml = new XML::Simple;
$data = $xml->XMLin($ARGV[0])->{match};

print STDERR "$data\n";

# iterate over the match entries
foreach $row (keys(%$data)) {
    $m = $data->{$row} or next;
    
    $src = $m->{src};
    $tgt = $m->{tgt};

    $xfstr = $m->{xf};
    @xf = split(' ', $xfstr);

    $theta = atan2(-$xf[1], $xf[0]);
    $tx    = $xf[3];
    $ty    = $xf[7];

    # query for equivalent match already in the database
    $sql  = ""; # "SELECT NULL INTO \@pid;\n"
    $sql .= "SELECT fnProposal('$tgt', '$src', $theta, $tx, $ty) INTO \@pid;\n";

    foreach $a (keys(%attr)) {
        if (exists $m->{$a}) {
            $aid = $attr{$a};
            $val = $m->{$a};
            if (($aid == 1) || ($aid == 5)) {
                $sql .= "CALL procAttr(\@pid, $aid, \@mid, $val, '${xfstr}');\n";
            } else {
                $sql .= "CALL procAttr(\@pid, $aid, \@mid, $val, '');\n";
            }
        }
    }

    if ((exists($m->{status}) && $m->{status} > 0) || exists($m->{comment})) {
        $status  = exists($m->{status})  ? $m->{status}  : 0;
        $comment = exists($m->{comment}) ? $m->{comment} : '';
        $status  = $statuses[$status];

        if (($status >= 0) || ($comment ne '')) {
            $sql .= "REPLACE INTO tblEvaluations (pid, uid, eval, txt) VALUES " .
                "(\@pid, \@uid, $status, '$comment');\n";
        }
    }

    print $sql . "\n";
}
