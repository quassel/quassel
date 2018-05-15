#!/usr/bin/perl

# This script scans the Quassel source for required icons and imports the needed
# icons (and only them) from a full icon theme.
# Additional icons can be specified in whitelisted-icons; you can also blacklist icons.
#
# NOTE: Unless you are a Quassel developer and need to bump the icons we ship, you shouldn't
#       need to use this script!

# USAGE: ./import/import_theme.pl $srcthemedir $themename
#
# Examples:
#   import_theme.pl ~/oxygen-icons oxygen
#   import_theme.pl ~/breeze-icons/icons breeze
#   import_theme.pl ~/breeze-icons/icons-dark breeze-dark

use strict;
use warnings;

use File::Basename;
use File::Find;
use File::Spec;

my $scriptroot = File::Basename::dirname(File::Spec->rel2abs($0));
my $srcroot = File::Spec->catdir($scriptroot, "..", "..");
my $srcdir = File::Spec->catdir($srcroot, "src");

my $srcthemedir = shift;
my $themename = shift;

# Sanity checks
die "Theme directory must be given\n" unless length $srcthemedir;
die "Theme directory \"$srcthemedir\" not found\n" unless -e $srcthemedir and -d $srcthemedir;
die "Theme name must not be empty\n" unless length $themename;

my $destbasedir  = File::Spec->catdir($srcroot, "3rdparty", "icons");
my $destthemedir = File::Spec->catdir($destbasedir, $themename);

my $whitelistfile = "$scriptroot/whitelisted-icons";
my $blacklistfile = "$scriptroot/blacklisted-icons";

my %req_icons;
my %found_icons;
my %whitelist;
my %blacklist;

# Add whitelisted icons
open WHITELIST, "<$whitelistfile" or die "Could not open $whitelistfile\n";
while(<WHITELIST>) {
  s/#.*//;
  next unless my ($name) = /([-\w]+)\s*/;
  $req_icons{$name} = 1;
}
close WHITELIST;

# Load the icon blacklist
open BLACKLIST, "<$blacklistfile" or die "Could not open $blacklistfile\n";
while(<BLACKLIST>) {
  s/#.*//;
  next unless my ($name) = /([-\w]+)\s*/;
  $blacklist{$name} = 1;
}
close BLACKLIST;

# We now grep the source for QIcon::fromTheme("fubar") to find required icons
print "Grepping $srcdir for required icons...\n";
my @results = `grep -r QIcon::fromTheme\\(\\" $srcdir`;
foreach(@results) {
  next unless my ($name) = /\W+QIcon::fromTheme\(\"([-\w]+)/;
  $req_icons{$name} = 1
    unless exists $blacklist{$name};
}

# Clean old output dir
print "Removing old $destthemedir...\n";
system("rm -rf $destthemedir");

# Now copy the icons
my %scalables;

print "Copying icons from $srcthemedir...\n";
opendir (BASEDIR, "$srcthemedir") or die "Could not open theme basedir\n";
my $scalableFound = 0;
foreach my $parent (readdir BASEDIR) {
  next unless (-d "$srcthemedir/$parent");
  $scalableFound = $scalableFound ? 1 : $parent eq 'scalable';
  next if $parent eq '.' or $parent eq '..' or $parent eq 'scalable' or $parent =~ /\..*/;
  my $ischildcat = $parent =~ /\d+x\d+/ ? 1 : 0;
  opendir (SIZEDIR, "$srcthemedir/$parent") or die "Could not open dir $parent\n";
  foreach my $child (readdir SIZEDIR) {
    next if $child eq '.' or $child eq '..';
    my $cat = $ischildcat ? $child : $parent;
    opendir (CATDIR, "$srcthemedir/$parent/$child") or die "Could not open category dir\n";
    foreach my $icon (readdir CATDIR) {
      my $iconname = $icon;
      $iconname =~ s/\.png$//;
      $iconname =~ s/\.svg$//;
      next unless exists $req_icons{$iconname};
      $scalables{$cat}{$iconname} = 1;
      system "mkdir -p $destthemedir/$parent/$child" and die "Could not create category dir\n";
      system "cp -aL $srcthemedir/$parent/$child/$icon $destthemedir/$parent/$child"
        and die "Error while copying file $parent/$child/$icon\n";
      print "Copy: $srcthemedir/$parent/$child/$icon\n";
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
    system "mkdir -p $destthemedir/scalable/$cat" and die "Could not create category dir\n";
    foreach my $scalable (keys %{$scalables{$cat}}) {
      system "cp -aL $srcthemedir/scalable/$cat/$scalable.svgz $destthemedir/scalable/$cat/$scalable.svgz";
    }
  }
}

# Warn if we have still icons left
foreach my $icon (keys %req_icons) {
  next if defined $found_icons{$icon};
  print "Warning: Missing icon $icon\n";
}

# Copy license etc.
system "cp -aL $srcthemedir/AUTHORS* $srcthemedir/CONTRIBUTING* $srcthemedir/COPYING* $srcthemedir/index.theme $destthemedir/ | true";

# Generate .qrc
#my $qrcfile = $themename."_icon_theme.qrc";
#$qrcfile =~ s/-/_/g;
my $qrcfile = File::Spec->catdir($destbasedir, ($themename."_icon_theme.qrc") =~ s/-/_/gr);
print "Generating $qrcfile...\n";

my @file_list = ();

sub push_icon_path {
  return unless /\.png$/ or /\.svg$/ or /^index.theme$/;
  push @file_list, "    <file>".$File::Find::name =~ s|^$destbasedir/||gr."</file>";
}

find(\&push_icon_path, $destthemedir);
@file_list = sort(@file_list);
my $files = join "\n", @file_list;

my $qrc = "<RCC>\n"
         ."  <qresource prefix=\"/icons\">\n"
         ."$files\n"
         ."  </qresource>\n"
         ."</RCC>\n";

open QRC, ">$qrcfile" or die "Could not open $qrcfile for writing!\n";
print QRC $qrc;
close QRC;

print "Done.\n";
