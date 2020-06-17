#!/usr/bin/perl -w
use strict;
use lib 'mod';
use Ujsonin;
use Data::Dumper;

my $data = qq|{
    "x": "blah",
    "y1": "fsdfse",
    za: "zs",
    arr: [ "1",2,"3" ],
    "sub": { // "x": "1"
        "z": // "24"
          "23"
        "empty": "" /*
        "a": "4" */
    },
    tf: false,
    num: 10,
    neg: -35
}|;
Ujsonin::init();
my $root = Ujsonin::parse( $data, 0 );
print Dumper( $root );