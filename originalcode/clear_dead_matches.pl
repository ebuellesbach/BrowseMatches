#!/usr/bin/env perl -w

# clear_dead_matches.pl
# Benedict Brown
#
# simple script to clear proposals from the database that have no attributes
#
# argument 0 is the host
# argument 1 is the username
# argument 2 is the password
# argument 3 is the database name

use DBI;

$host = $ARGV[0];
$user = $ARGV[1];
$pass = $ARGV[2];
$name = $ARGV[3];

$dsn = "DBI:mysql:database=$name;host=$host";
$dbh = DBI->connect($dsn, $user, $pass);

my $query = $dbh->prepare("SELECT pid FROM tblProposals NATURAL LEFT JOIN tblProposalAttributes WHERE aid IS NULL;");
$query->execute();

@pids = ();
while (my $ref = $query->fetchrow_arrayref) {
    push @pids, $$ref[0];
}

for $pid (@pids) {
    $dbh->do("DELETE FROM tblProposals WHERE pid = $pid;\n");
}

