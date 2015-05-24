#!/usr/bin/perl -W


use strict;
use DBI;
use Text::CSV;

my $file="summitslist.csv";
#my $file="test.csv";

my $dbh = DBI->connect("dbi:SQLite:dbname=./summits.sqlite", "", "", {},) or die $DBI::errstr;

my $csv = Text::CSV->new({ sep_char => ',', binary  => 1, allow_loose_quotes => 1, escape_char => '' });
open(my $data, '<', $file) or die "Could not open '$file' $!\n";

while (my $line = <$data>) {
#	chomp $line;
#	print $line; 
	$line =~ s/""/"/g;
	if ($csv->parse($line)) {
		my @fields = $csv->fields();
		if (scalar (@fields) > 16) {
			my $query="INSERT INTO summits (SummitCode,AssociationName,RegionName,SummitName,AltM,AltFt,GridRef1,GridRef2,Longitude,Latitude,Points,BonusPoints,ValidFrom,ValidTo,ActivationCount,ActivationDate,ActivationCall) VALUES (\"$fields[0]\", \"$fields[1]\", \"$fields[2]\", \"$fields[3]\", $fields[4], $fields[5], \"$fields[6]\", \"$fields[7]\", $fields[8], $fields[9], $fields[10], $fields[11], \"$fields[12]\", \"$fields[13]\", $fields[14], \"$fields[15]\", \"$fields[16]\");";
#			print ("$query\n");
			my $sth = $dbh->prepare($query);
			if ($sth) {
				$sth->execute();
				$sth->finish();
			} else {
				warn "Could not insert record $line\n";
			}
		}
	} else {
		warn "Line could not be parsed: $line\n";
	}
}

$dbh->disconnect();

