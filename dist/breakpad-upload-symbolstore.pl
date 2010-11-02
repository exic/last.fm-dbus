#!/usr/bin/perl -w

use strict;
use LWP::UserAgent;
use HTTP::Request::Common;


open FILE, $ARGV[0] or die $!;
binmode FILE;
my ($buf, $data, $n);
while (($n = read FILE, $data, 1024 * 64) != 0) {
    $buf .= $data;
}


my $agent = LWP::UserAgent->new( agent => 'perl post' );
my $response = $agent->request( POST 'http://oops.last.fm:80/symbols/upload',
        Content_Type => 'application/octet-stream',
        Content => $buf );

print $response->error_as_HTML unless $response->is_success;
print $response->as_string;
