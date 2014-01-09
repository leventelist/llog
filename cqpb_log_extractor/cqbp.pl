#!/usr/bin/perl -w

use strict;
use Geo::Distance::XS;
use Ham::Locator;
use Math::Round;
use Text::CSV;

my $my_category="B";

my $ssb_mult=2;
my $cw_mult=3;

#llog stuff
my $llog_fields=21;
my $llog_QRA_field=6;
my $llog_MODE_field=9;
my $llog_MYQRA_field=16;


my $csv = Text::CSV->new({ sep_char => ',' });
my $geo = Geo::Distance->new;

#setup geo::distance
$geo->formula('hsin');
my $roundend_distance;

my $my_loc = new Ham::Locator;
my $his_loc = new Ham::Locator;

#CSV stuff
my $file = $ARGV[0] or die "Need to get CSV file on the command line\n";
open(my $data, '<', $file) or die "Could not open '$file' $!\n";
my @fields;
my $ptr=\@fields;
my $numfields;

#output file
open(my $out_data, '>', "out.csv") or die "Could not open 'out.csv' $!\n";

#some crap
print $out_data "Category,$my_category\n";

#write CSV header
push (@fields, "YYYY-MM-DD");
push (@fields, "UTC");
push (@fields, "CALL");
push (@fields, "TXRST");
push (@fields, "RXRST");
push (@fields, "QTH");
push (@fields, "QRA");
push (@fields, "NAME");
push (@fields, "QRG");
push (@fields, "MODE");
push (@fields, "RXNR");
push (@fields, "TXNR");
push (@fields, "COMMENT");
push (@fields, "QSL");
push (@fields, "LOCAL_CALL");
push (@fields, "LOCAL_QTH");
push (@fields, "LOCAL_QRA");
push (@fields, "LOCAL_ASL");
push (@fields, "LOCAL_RIG");
push (@fields, "LOCAL_PWR");
push (@fields, "LOCAL_ANT");
push (@fields, "QRB");
push (@fields, "MULTIPLIER");
push (@fields, "SCORE");

$csv->print($out_data, $ptr);
print $out_data "\n";

#score
my $total_score=0;
my $score=0;

while (my $line = <$data>) {
	if ($csv->parse($line)) {
		@fields = $csv->fields();
		$numfields=scalar(@fields);
		if ($numfields==$llog_fields) {
			$my_loc->set_loc($fields[$llog_MYQRA_field]);
			$his_loc->set_loc($fields[$llog_QRA_field]);
			my ($my_latitude, $my_longitude) = $my_loc->loc2latlng;
			my ($his_latitude, $his_longitude) = $his_loc->loc2latlng;
			if (defined $my_latitude && defined $my_longitude && defined $his_latitude && defined $his_longitude ) {
				my $distance = $geo->distance('kilometer', $my_longitude, $my_latitude => $his_longitude, $his_latitude);
				my $roundend_distance=nearest(1, $distance);
				if ($roundend_distance<5) {
					$roundend_distance=5;
				}
				push (@fields, $roundend_distance);
				my $mult;
				if ($fields[$llog_MODE_field]=~/SSB/i || $fields[$llog_MODE_field]=~/USB/i) {
					$mult=$ssb_mult;
				} elsif ($fields[$llog_MODE_field]=~/CW/i){
					$mult=$cw_mult;
				} else {
					$mult=1;
				}
				push (@fields, $mult);
				$score=$mult*$roundend_distance;
				$total_score+=($score);
				push (@fields, $score);
			} else {
				push (@fields, 0);
				push (@fields, 1);
				push (@fields, 0);
				print STDERR "QRA ERROR!!! Line skipped\n";
			}
			$csv->print($out_data, $ptr);
			print $out_data "\n";
		}
	}
}

print $out_data "Total score,$total_score\n";
print "Output written to out.csv\n\n";


