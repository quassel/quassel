#!/usr/bin/perl

# This script scans the Quassel source for requested icons and imports the needed
# icons (and only them) from a KDE theme (by default Oxygen).
# This relies on all icons being requested using one of the convenience constructors in
# (K)IconLoader, like this:
#   widget->setIcon(SmallIcon("fubar"));
# Additional icons can be specified in whitelist-icons; you can also blacklist icons.
#
# NOTE: Unless you are a Quassel developer and need to bump the icons we ship, you shouldn't
#       need to use this script!

# USAGE: ./import/import_theme.pl $systhemefolder $themename $parentFolderFileSuffix
# Run from the icon/ directory.
#
# Examples: (being inside the icons folder)
# ./import/import_theme.pl ~/oxygen-icons oxygen
# ./import/import_theme.pl ~/breeze-icons/icons breeze ICONS
# ./import/import_theme.pl ~/breeze-icons/icons-dark breezedark ICONS

use strict;
use warnings;
use Data::Dumper;
use File::Find;

my $themefolder = shift;

my $source = "../src";
my $themename = shift;
$themename = $themename ? $themename : "oxygen";
my $qrcfile_kde = $themename . ".qrc";
my $parentFolderFileSuffix = shift;

my $whitelistfile = "import/whitelist-icons";
my $blacklistfile = "import/blacklisted-icons";
my $extrafile = "import/extra-icons.qrc." . $themename;

my %req_icons;
my %found_icons;
my %blacklist;
my %themeblacklist;
my %whitelist;
my $extrafilecontent;

# First, load the icon blacklist
open BLACKLIST, "<$blacklistfile" or die "Could not open $blacklistfile\n";
while(<BLACKLIST>) {
  s/#.*//;
  next unless my ($name) = /([-\w]+)\s*/;
  $blacklist{$name} = 1;
}
close BLACKLIST;

my $hasthemeblacklist = 1;
open BLACKLIST, "<$blacklistfile.$themename" or $hasthemeblacklist = 0;
if ($hasthemeblacklist) {
  while(<BLACKLIST>) {
    s/#.*//;
    next unless my ($name) = /([-\w]+)\s*/;
    $blacklist{$name} = 1;
    $themeblacklist{$name} = 1;
  }
  close BLACKLIST;
} else {
  print "Info: No theme specific blacklist found...\n";
}

# We now grep the source for things like SmallIcon("fubar") and generate size and name from that
print "Grepping $source for requested icons...\n";
my @results = `grep -r QIcon::fromTheme\\(\\" $source`;
foreach(@results) {
  next unless my ($name) = /\W+QIcon::fromTheme\(\"([-\w]+)/;
  $req_icons{$name} = 1
    unless exists $blacklist{$name};
}

# Add whitelist icons
open WHITELIST, "<$whitelistfile" or die "Could not open $whitelistfile\n";
while(<WHITELIST>) {
  s/#.*//;
  next unless my ($name) = /([-\w]+)\s*/;
  $req_icons{$name} = 1
    unless exists $themeblacklist{$name};
}
close WHITELIST;

# Read in extra-icons
my $hasthemeextrafile = 1;
local $/;
open EXTRAFILE, "<$extrafile" or $hasthemeextrafile = 0;
if($hasthemeextrafile) {
  binmode EXTRAFILE;
  $extrafilecontent = <EXTRAFILE>;
  close EXTRAFILE;
} else {
  $extrafilecontent = "";
}

# Clean old output dir
print "Removing old $themename...\n";
system("rm -rf $themename");

# Now copy the icons
my %scalables;

print "Copying icons from $themefolder...\n";
opendir (BASEDIR, "$themefolder") or die "Could not open theme basedir\n";
my $scalableFound = 0;
foreach my $parent (readdir BASEDIR) {
  next unless (-d "$themefolder/$parent");
  $scalableFound = $scalableFound ? 1 : $parent eq 'scalable';
  next if $parent eq '.' or $parent eq '..' or $parent eq 'scalable' or $parent =~ /\..*/;
  my $ischildcat = $parent =~ /\d+x\d+/ ? 1 : 0;
  opendir (SIZEDIR, "$themefolder/$parent") or die "Could not open dir $parent\n";
  foreach my $child (readdir SIZEDIR) {
    next if $child eq '.' or $child eq '..';
    my $cat = $ischildcat ? $child : $parent;
    opendir (CATDIR, "$themefolder/$parent/$child") or die "Could not open category dir\n";
    foreach my $icon (readdir CATDIR) {
      my $iconname = $icon;
      $iconname =~ s/\.png$//;
      $iconname =~ s/\.svg$//;
      next unless exists $req_icons{$iconname};
      $scalables{$cat}{$iconname} = 1;
      system "mkdir -p $themename/$parent/$child" and die "Could not create category dir\n";
      system "cp -aL $themefolder/$parent/$child/$icon $themename/$parent/$child"
        and die "Error while copying file $parent/$child/$icon\n";
      #print "Copy: $themefolder/$parent/$child/$icon\n";
      $found_icons{$iconname} = 1;
    }
    closedir CATDIR;
  }
  closedir SIZEDIR;
}
closedir BASEDIR;

# Copy scalables
if ($scalableFound) {
  foreach my $cat (keys %scalables) {
    system "mkdir -p $themename/scalable/$cat" and die "Could not create category dir\n";
    foreach my $scalable (keys %{$scalables{$cat}}) {
      system "cp -aL $themefolder/scalable/$cat/$scalable.svgz $themename/scalable/$cat/$scalable.svgz";
    }
  }
}

# Warn if we have still icons left
foreach my $icon (keys %req_icons) {
  next if defined $found_icons{$icon};
  print "Warning: Missing icon $icon\n";
}

# Copy license etc.
system "cp $themefolder/AUTHORS $themefolder/CONTRIBUTING $themefolder/COPYING* $themefolder/index.theme $themename/";

if($parentFolderFileSuffix) {
	system "cp $themefolder/../AUTHORS*$parentFolderFileSuffix $themefolder/../CONTRIBUTING*$parentFolderFileSuffix $themefolder/../COPYING*$parentFolderFileSuffix $themename/";
}

# Generate .qrc
my @file_list;
generate_qrc($themename, $qrcfile_kde, $extrafilecontent);

print "Done.\n";

########################################################################################
sub generate_qrc {
  my $dir = shift;
  my $qrcfile = shift;

  @file_list = ();
  find(\&push_icon_path, $dir);
  @file_list = sort(@file_list );
  my $files = join "\n", @file_list;

  my $qrc = "<RCC>\n"
           ."  <qresource prefix=\"/icons\">\n"
           ."$files\n"
           ."$extrafilecontent\n"
           ."  </qresource>\n"
           ."</RCC>\n";

  open QRC, ">$qrcfile" or die "Could not open $qrcfile for writing!\n";
  print QRC $qrc;
  close QRC;
}

sub push_icon_path {
  return unless /\.png$/ or /\.svg$/ or /^index.theme$/;

  push @file_list, "    <file>$File::Find::name</file>";
}
