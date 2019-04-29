#!/usr/bin/perl -w

$id = 1;
while (<STDIN>) {
    if (/<match /) {
        s/<match /<match id="$id" /;
        $id++;
    }

    print $_;
}
