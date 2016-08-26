#!/usr/bin/env perl

#This script generates eprom data for the FK115 amateur radio.
#By HA5OGL. ha5ogl.levente@gmail.com

use strict;

#constants
#
#Frequency of the on board TCXO
my $f_tcxo=12E6;

#Onboard divider
my $tcxo_fixed_divider=8;

#Divider of the PLL prescaler
my $prescaler=64;

#IF frequency 21.6MHz
my $f_if=21.6E6;

#raster frequency. Let it be 12.5kHz
#Unit test. Example from the manual
#my $f_r=10E3;

my $f_r=12500;

####

my $fh;

open $fh, "+>", "eprom.srec" or die $!;


#calculate and write EPROM program
#                  ch   RX_F      TX_F

#repeaters for 7.6MHz shif
my $ch=0;

print("CH, RX, TX [Hz]\n");
print("\nRepeaters (7.6MHz shift)\n\n");
for (my $repeater=438.425E6; $repeater<=439.575E6; $repeater+=$f_r) {

	my $rx=$repeater;
	my $tx=$repeater-7.6E6;
	calculate_dividers($ch, $rx, $tx);
	my $ch_vis=$ch+1;
	print ("$ch_vis, $rx, $tx\n");
	$ch++;
}


#432.994 – 433.381MHz
#print("\nRepeaters (1.6MHz shift)\n\n");
#for (my $repeater=433.0E6; $repeater<=433.575E6; $repeater+=12.5E3) {
#
#	my $rx=$repeater;
#	my $tx=$repeater-1.6E6;
#	calculate_dividers($ch, $rx, $tx);
#	print ("$ch, $rx, $tx\n");
#	$ch++;
#}

print("\nSimplex channels\n\n");

#433.394 – 433.600MHz

for (my $simplex=433.4E6; $simplex<=434.600E6; $simplex+=$f_r) {

	my $rx=$simplex;
	my $tx=$simplex;
	calculate_dividers($ch, $rx, $tx);
	my $ch_vis=$ch+1;
	print ("$ch_vis, $rx, $tx\n");
	$ch++;
}

#Unit test. Example from the manual
#calculate_dividers(1, 160.31E6, 160.31E6);
#write some trailer

print $fh "-output eprom.hex -intel\n";

close $fh;

#generate the files
#First, interpret the srec
system "srec_cat \@eprom.srec";
#generate binary output from the hex.
#You can view it with hexdump -C eprom.bin -or-
#od -Ax -w16 -t x1z eprom.bin
system "srec_cat eprom.hex -intel -output eprom.bin -binary";

sub calculate_dividers {

	my $ch=shift;
	my $f_rx=shift;
	my $f_tx=shift;

#	print("RX=$f_rx TX=$f_tx\n");
	my $f_rx_vco=$f_rx-$f_if;

	my $adr=$ch * 16; #calculate eprom address from channel number
	get_registers_by_frequencies($adr, $f_rx_vco);
	my $adr=$adr + 8; #calculate eprom address from channel number
	get_registers_by_frequencies($adr, $f_tx);

}

sub write_srec_file {
	my $ADR=shift;
	my $A=shift;
	my $N=shift;
	my $R=shift;

	my $data;
	my $end;

#generate the A data. This is stored in 2 bytes
	$data = $A & 0x0f;
	$end=$ADR+1;
	print $fh "-generate $ADR $end --constant $data\n";

	$ADR++;	#increment the address
	$end=$ADR+1;
	$data = ($A >> 4) & 0x0f;
	print $fh "-generate $ADR $end --constant $data\n";

#generate the N data. 3 bytes.

	$ADR++;
	$end=$ADR+1;
	$data = $N & 0x0f;
	print $fh "-generate $ADR $end --constant $data\n";

	$ADR++;	#increment the address
	$end=$ADR+1;
	$data = ($N >> 4) & 0x0f;
	print $fh "-generate $ADR $end --constant $data\n";

	$ADR++;	#increment the address
	$end=$ADR+1;
	$data = ($N >> 8) & 0x0f;
	print $fh "-generate $ADR $end --constant $data\n";

#generate the R data. 3 bytes. 
	
	$ADR++;
	$end=$ADR+1;
	$data = $R & 0x0f;
	print $fh "-generate $ADR $end --constant $data\n";

	$ADR++;	#increment the address
	$end=$ADR+1;
	$data = ($R >> 4) & 0x0f;
	print $fh "-generate $ADR $end --constant $data\n";

	$ADR++;	#increment the address
	$end=$ADR+1;
	$data = ($R >> 8) & 0x0f;
	print $fh "-generate $ADR $end --constant $data\n";

}

sub get_registers_by_frequencies {

	my $adr=shift;
	my $freq=shift;

	my $D=calculate_D($freq);
	my $N=calculate_N($D);
	my $A=calculate_A($D);
	my $R=calculate_R($f_r);

#	print ("N=$N A=$A R=$R\n");

	write_srec_file($adr, $A, $N, $R);

}

sub calculate_D {

	my $freq=shift;

	return $freq/$f_r;
}

#main divider
sub calculate_N {

	my $D=shift;
	return int $D/$prescaler;
}

#swallow counter
sub calculate_A {
	my $D=shift;
	return $D % $prescaler;

}

#reference divider
sub calculate_R {
	my $freq=shift;

	return $f_tcxo/$tcxo_fixed_divider/$freq;

}

