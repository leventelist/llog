#!/usr/bin/perl -w

use strict;
use Geo::Distance::XS;
use Ham::Locator;
use Math::Round;
use Text::CSV;
use Math::Trig;
use Math::Trig 'great_circle_direction';

my $my_category="B";

my $ssb_mult=2;
my $cw_mult=3;

#llog stuff
my $llog_fields=23;
my $llog_QRA_field=6;
my $llog_MODE_field=9;
my $llog_MYQRA_field=18;
my $llog_COMMENT_field=14;

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
push (@fields, "RX_EXTRA");
push (@fields, "TX_EXTRA");
push (@fields, "COMMENT");
push (@fields, "QSL");
push (@fields, "LOCAL_CALL");
push (@fields, "LOCAL_QTH");
push (@fields, "LOCAL_QRA");
push (@fields, "LOCAL_ASL");
push (@fields, "LOCAL_RIG");
push (@fields, "LOCAL_PWR");
push (@fields, "LOCAL_ANT");
push (@fields, "LOCAL_NAME");
#push (@fields, "AZIMUTH");
#push (@fields, "LOCAL_LAT");
#push (@fields, "LOCAL_LON");
#push (@fields, "FAR_LAT");
#push (@fields, "FAR_LON");
#push (@fields, "MULTIPLIER");
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
			if ($fields[$llog_COMMENT_field]=~/YL/i) {
				$score=5;
			} else {
				$score=1;
			}
			$total_score+=($score);
			push (@fields, "Levente");
			push (@fields, $score);
			$csv->print($out_data, $ptr);
			print $out_data "\n";
		}
	}
}

print $out_data "Total score,$total_score\n";
print "Output written to out.csv\n\n";


