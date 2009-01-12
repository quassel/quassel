#!/usr/bin/perl

# Take mIRC's servers.ini and create a networks.ini suitable for Quassel.

use strict;

my $serverlist = {};

open SERVERS_INI, "<servers.ini" or die "Could not open servers.ini";
while(<SERVERS_INI>) {
  my ($host, $portrange, $net) = /SERVER:(.+):(.+)GROUP:(.+)\r\n/;
  if($host) {
    foreach(split /,/, $portrange) {
      s/(\d+)-\d+/$1/;
      push @{$serverlist->{$net}}, { Host => $host, Port => $_};
    }
  }
}
close SERVERS_INI;

open NETWORKS_INI, ">networks.ini" or die "Could not open networks.ini for writing";
foreach(sort keys %$serverlist) {
  print NETWORKS_INI "[$_]\n";
  my @servers;
  foreach(@{$serverlist->{$_}}) {
    push @servers, "$_->{Host}:$_->{Port}";
  }
  print NETWORKS_INI "Servers=", join ',', @servers;
  print NETWORKS_INI "\n\n";
}
close NETWORKS_INI;

print "Done.\n";
