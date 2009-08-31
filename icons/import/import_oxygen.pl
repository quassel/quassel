#!/usr/bin/perl

# This script scans the Quassel source for requested icons and imports the needed
# icons (and only them) from KDE's Oxygen theme.
# This relies on all icons being requested using one of the convenience constructors in
# (K)IconLoader, like this:
#   widget->setIcon(SmallIcon("fubar"));
# Additional icons can be specified in extra-icons; you can also blacklist icons.
#
# NOTE: Unless you are a Quassel developer and need to bump the icons we ship, you shouldn'y
#       need to use this script!

# USAGE: ./import/import_oxygen.pl $systhemefolder
# Run from the icon/ directory.

use strict;
use Data::Dumper;
use File::Find;

my $oxygen = shift;

my $source = "../src";
my $quassel_icons = "oxygen";
my $output = "oxygen_kde";
my $qrcfile_quassel = "oxygen.qrc";
my $qrcfile_kde = "oxygen_kde.qrc";

my $extrafile = "import/extra-icons";
my $blacklistfile = "import/blacklisted-icons";

my %sizes = (
        Desktop => 48,
        Bar => 22,
        MainBar => 22,
        Small => 16,
        Panel => 32,
        Dialog => 22
);

my %req_icons;
my %blacklist;
my %extra;

# First, load the icon blacklist
# Format: icon-name 16 22 32
open BLACKLIST, "<$blacklistfile" or die "Could not open $blacklistfile\n";
while(<BLACKLIST>) {
  s/#.*//;
  next unless my ($name, $sizes) = /([-\w]+)\s+(\d+(?:\s+\d+)*)?/;
  $blacklist{$name} = $sizes;
}
close BLACKLIST;

# We now grep the source for things like SmallIcon("fubar") and generate size and name from that
print "Grepping $source for requested icons...\n";
my @results = `grep -r Icon\\(\\" $source`;
foreach(@results) {
  next unless my ($type, $name) = /\W+(\s|Desktop|Bar|MainBar|Small|Panel|Dialog)Icon\("([-\w]+)/;
  $type = "Desktop" if $type =~ /\s+/;
  my $size = $sizes{$type};
  $req_icons{$size}{$name} = 1
    unless exists $blacklist{$name} and ($blacklist{$name} == undef or $blacklist{$name} =~ /$size/);
}

# Add extra icons
open EXTRA, "<$extrafile" or die "Could not open $extrafile\n";
while(<EXTRA>) {
  s/#.*//;
  next unless my ($name, $sizes) = /([-\w]+)\s+(\d+(?:\s+\d+)*)/;
  foreach(split /\s+/, $sizes) {
    $req_icons{$_}{$name} = 1;
  }
}
close EXTRA;

# Clean old output dir
print "Removing old $output...\n";
system("rm -rf $output");

# Now copy the icons
my %scalables;

print "Copying icons from $oxygen...\n";
foreach my $size (keys %req_icons) {
  my $sizestr = $size.'x'.$size;
  opendir (BASEDIR, "$oxygen/$sizestr") or die "Could not open dir for size $size\n";
  foreach my $cat (readdir BASEDIR) {
    next if $cat eq '.' or $cat eq '..';
    system "mkdir -p $output/$sizestr/$cat" and die "Could not create category dir\n";
    system "mkdir -p $output/scalable/$cat" and die "Could not create category dir\n";
    opendir (CATDIR, "$oxygen/$sizestr/$cat") or die "Could not open category dir\n";
    foreach my $icon (readdir CATDIR) {
      $icon =~ s/\.png$//;
      next unless exists $req_icons{$size}{$icon};
      $scalables{"$cat/$icon"} = 1;
      system "cp -a $oxygen/$sizestr/$cat/$icon.png $output/$sizestr/$cat"
        and die "Error while copying file $sizestr/$cat/$icon.png\n";
      # print "Copy: $oxygen/$sizestr/$cat/$icon.png\n";
      delete $req_icons{$size}{$icon};
    }
    closedir CATDIR;
  }
  closedir BASEDIR;
}

# Copy scalables
foreach my $scalable (keys %scalables) {
  system "cp -a $oxygen/scalable/$scalable.svgz $output/scalable/$scalable.svgz";
}

# Warn if we have still icons left
foreach my $size (keys %req_icons) {
  foreach my $missing (keys %{ $req_icons{$size} }) {
    print "Warning: Missing icon $missing (size $size)\n";
  }
}

# Generate .qrc
my @file_list;
generate_qrc($quassel_icons, $qrcfile_quassel);
generate_qrc($output, $qrcfile_kde);

print "Done.\n";

########################################################################################
sub generate_qrc {
  my $dir = shift;
  my $qrcfile = shift;

  @file_list = ();
  find(\&push_icon_path, $dir);
  my $files = join "\n", @file_list;

  my $qrc = "<RCC>\n"
           ."  <qresource prefix=\"/icons\">\n"
           ."$files\n"
           ."  </qresource>\n"
           ."</RCC>\n";

  open QRC, ">$qrcfile" or die "Could not open $qrcfile for writing!\n";
  print QRC $qrc;
  close QRC;
}

sub push_icon_path {
  return unless /\.png$/;
  my $alias = $File::Find::name;
  $alias =~ s,^[^/]*(.*),$1,;

  push @file_list, "    <file alias=\"oxygen$alias\">$File::Find::name</file>";
}
