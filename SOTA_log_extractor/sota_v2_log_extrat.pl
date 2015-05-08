#!/usr/bin/perl -w

use strict;

require 'libllog.pl';

#use llog::libllog;

use Text::CSV;

#llog constants
my $llog_fields = 23;

my $llog_DATE_POS = 0;
my $llog_TIME_POS = 1;
my $llog_CALL_POS = 2;
my $llog_TXRST_POS = 3;
my $llog_RXRST_POS = 4;
my $llog_QTH_POS = 5;
my $llog_QRA_POS = 6;
my $llog_NAME_POS = 7;
my $llog_QRG_POS = 8;
my $llog_MODE_POS = 9;
my $llog_RXNR_POS = 10;
my $llog_TXNR_POS = 11;
my $llog_RXX_POS = 12;
my $llog_TXX_POS = 13;
my $llog_COMMENT_POS = 14;
my $llog_CSV_QSL_POS = 15;
my $llog_LOCAL_CALL_POS = 16;
my $llog_LOCAL_QTH_POS = 17;
my $llog_LOCAL_QRA_POS = 18;
my $llog_LOCAL_ALT_POS = 19;
my $llog_LOCAL_RIG_POS = 20;
my $llog_LOCAL_PWR_POS = 21;
my $llog_LOCAL_ANT_POS = 22;


my $csv = Text::CSV->new({ sep_char => ',' });

#CSV stuff
my $file = $ARGV[0] or die "Need to get CSV file on the command line\n";
open(my $data, '<', $file) or die "Could not open '$file' $!\n";
my @fields;
my $numfields;

#output file
open(my $out_data, '>', "out.csv") or die "Could not open 'out.csv' $!\n";

my @out_fields;
my $ptr=\@out_fields;

while (my $line = <$data>) {

	if ($csv->parse($line)) {
		@fields = $csv->fields();
		$numfields=scalar(@fields);
		if ($numfields==$llog_fields) {
			#let's split the date field. Ex. 2015-05-25 to 2015, 05 and 25.
			my $date=$fields[$llog_DATE_POS];
			my @date_fields=split('-', $date);
			my $year=$date_fields[0]%1000; #disgusting.
			my $month=$date_fields[1];
			my $day=$date_fields[2];
#			print "$year $month $day\n";

			#prepare the output
			@out_fields=("V2");
			push (@out_fields, $fields[$llog_LOCAL_CALL_POS]);
			push (@out_fields, $fields[$llog_TXX_POS]);
			push (@out_fields, "$day/$month/$year");
			push (@out_fields, $fields[$llog_TIME_POS]);
			push (@out_fields, $fields[$llog_QRG_POS]);
			push (@out_fields, $fields[$llog_MODE_POS]);
			push (@out_fields, $fields[$llog_CALL_POS]);
			push (@out_fields, $fields[$llog_RXX_POS]);
			push (@out_fields, $fields[$llog_COMMENT_POS]);
#			print "Out: @out_fields\n";

			#do the output
			$csv->print($out_data, $ptr);
			print $out_data "\n";

		} else { #supplied file is likely not a logfile.
		}
	}
}

print "Output written to out.csv\n\n";

