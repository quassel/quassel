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
my $output = "oxygen";
my $qrcfile_kde = "oxygen.qrc";

my $extrafile = "import/extra-icons";
my $blacklistfile = "import/blacklisted-icons";

my %req_icons;
my %found_icons;
my %blacklist;
my %extra;

# First, load the icon blacklist
open BLACKLIST, "<$blacklistfile" or die "Could not open $blacklistfile\n";
while(<BLACKLIST>) {
  s/#.*//;
  next unless my ($name) = /([-\w]+)\s*/;
  $blacklist{$name} = 1;
}
close BLACKLIST;

# We now grep the source for things like SmallIcon("fubar") and generate size and name from that
print "Grepping $source for requested icons...\n";
my @results = `grep -r QIcon::fromTheme\\(\\" $source`;
foreach(@results) {
  next unless my ($name) = /\W+QIcon::fromTheme\(\"([-\w]+)/;
  $req_icons{$name} = 1
    unless exists $blacklist{$name};
}

# Add extra icons
open EXTRA, "<$extrafile" or die "Could not open $extrafile\n";
while(<EXTRA>) {
  s/#.*//;
  next unless my ($name) = /([-\w]+)\s*/;
  $req_icons{$name} = 1;
}
close EXTRA;

# Clean old output dir
print "Removing old $output...\n";
system("rm -rf $output");

# Now copy the icons
my %scalables;

print "Copying icons from $oxygen...\n";
opendir (BASEDIR, "$oxygen") or die "Could not open oxygen basedir\n";
foreach my $sizestr (readdir BASEDIR) {
  next unless $sizestr =~ /\d+x\d+/;
  opendir (SIZEDIR, "$oxygen/$sizestr") or die "Could not open dir $sizestr\n";
  foreach my $cat (readdir SIZEDIR) {
    next if $cat eq '.' or $cat eq '..';
    opendir (CATDIR, "$oxygen/$sizestr/$cat") or die "Could not open category dir\n";
    foreach my $icon (readdir CATDIR) {
      $icon =~ s/\.png$//;
      next unless exists $req_icons{$icon};
      $scalables{$cat}{$icon} = 1;
      system "mkdir -p $output/$sizestr/$cat" and die "Could not create category dir\n";
      system "cp -a $oxygen/$sizestr/$cat/$icon.png $output/$sizestr/$cat"
        and die "Error while copying file $sizestr/$cat/$icon.png\n";
      #print "Copy: $oxygen/$sizestr/$cat/$icon.png\n";
      $found_icons{$icon} = 1;
    }
    closedir CATDIR;
  }
  closedir SIZEDIR;
}
closedir BASEDIR;

# Copy scalables
foreach my $cat (keys %scalables) {
  system "mkdir -p $output/scalable/$cat" and die "Could not create category dir\n";
  foreach my $scalable (keys %scalables{$cat}) {
    system "cp -a $oxygen/scalable/$cat/$scalable.svgz $output/scalable/$cat/$scalable.svgz";
  }
}

# Warn if we have still icons left
foreach my $icon (keys %req_icons) {
  next if defined $found_icons{$icon};
  print "Warning: Missing icon $icon\n";
}

# Copy license etc.
system "cp $oxygen/AUTHORS $oxygen/CONTRIBUTING $oxygen/COPYING $oxygen/index.theme $output/";

# Generate .qrc
my @file_list;
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
  return unless /\.png$/ or /^index.theme$/;

  push @file_list, "    <file>$File::Find::name</file>";
}
