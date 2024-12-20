BEGIN TRANSACTION;


CREATE TABLE station (
	name TEXT,
	CALL TEXT,
	OPERATOR_CALL TEXT,
	OPERATOR_NAME TEXT,
	QTH TEXT,
	QRA TEXT,
	ASL TEXT,
	rig TEXT,
	ant TEXT,
	comment TEXT
);


CREATE TABLE log (
	date TEXT,
	UTC TEXT,
	call TEXT,
	rxrst TEXT,
	txrst TEXT,
	rxnr INTEGER,
	txnr INTEGER,
	rxextra TEXT,
	txextra TEXT,
	QTH TEXT,
	name TEXT,
	QRA TEXT,
	QRG float,
	mode TEXT,
	pwr TEXT,
	rxQSL INTEGER,
	txQSL INTEGER,
	SOTA_REF TEXT,
	S2S_REF TEXT,
	comment TEXT,
	station INTEGER
);


CREATE TABLE mode (
	name TEXT,
	default_rst TEXT default "599",
	super_mode TEXT default "",
	comment TEXT default ""
);


/* INSERT QUERY NO: 1 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'8PSK125', 'PSK', '599'
);

/* INSERT QUERY NO: 2 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'8PSK125F', 'PSK', '599'
);

/* INSERT QUERY NO: 3 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'8PSK125FL', 'PSK', '599'
);

/* INSERT QUERY NO: 4 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'8PSK250', 'PSK', '599'
);

/* INSERT QUERY NO: 5 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'8PSK250F', 'PSK', '599'
);

/* INSERT QUERY NO: 6 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'8PSK250FL', 'PSK', '599'
);

/* INSERT QUERY NO: 7 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'8PSK500', 'PSK', '599'
);

/* INSERT QUERY NO: 8 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'8PSK500F', 'PSK', '599'
);

/* INSERT QUERY NO: 9 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'8PSK1000', 'PSK', '599'
);

/* INSERT QUERY NO: 10 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'8PSK1000F', 'PSK', '599'
);

/* INSERT QUERY NO: 11 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'8PSK1200F', 'PSK', '599'
);

/* INSERT QUERY NO: 12 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'AMTORFEC', 'TOR', '599'
);

/* INSERT QUERY NO: 13 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'ASCI', 'RTTY', '599'
);

/* INSERT QUERY NO: 14 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'CHIP64', 'CHIP', '599'
);

/* INSERT QUERY NO: 15 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'CHIP128', 'CHIP', '599'
);

/* INSERT QUERY NO: 16 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'DOM-M', 'DOMINO', '599'
);

/* INSERT QUERY NO: 17 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'DOM4', 'DOMINO', '599'
);

/* INSERT QUERY NO: 18 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'DOM5', 'DOMINO', '599'
);

/* INSERT QUERY NO: 19 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'DOM8', 'DOMINO', '599'
);

/* INSERT QUERY NO: 20 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'DOM11', 'DOMINO', '599'
);

/* INSERT QUERY NO: 21 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'DOM16', 'DOMINO', '599'
);

/* INSERT QUERY NO: 22 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'DOM22', 'DOMINO', '599'
);

/* INSERT QUERY NO: 23 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'DOM44', 'DOMINO', '599'
);

/* INSERT QUERY NO: 24 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'DOM88', 'DOMINO', '599'
);

/* INSERT QUERY NO: 25 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'DOMINOEX', 'DOMINO', '599'
);

/* INSERT QUERY NO: 26 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'DOMINOF', 'DOMINO', '599'
);

/* INSERT QUERY NO: 27 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'FMHELL', 'HELL', '599'
);

/* INSERT QUERY NO: 28 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'FSK31', 'PSK', '599'
);

/* INSERT QUERY NO: 29 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'FSKHELL', 'HELL', '599'
);

/* INSERT QUERY NO: 30 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'FSQCALL', 'MFSK', '599'
);

/* INSERT QUERY NO: 31 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'FST4', 'MFSK', '599'
);

/* INSERT QUERY NO: 32 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'FST4W', 'MFSK', '599'
);

/* INSERT QUERY NO: 33 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'FT4', 'MFSK', '599'
);

/* INSERT QUERY NO: 34 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'GTOR', 'TOR', '599'
);

/* INSERT QUERY NO: 35 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'HELL80', 'HELL', '599'
);

/* INSERT QUERY NO: 36 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'HELLX5', 'HELL', '599'
);

/* INSERT QUERY NO: 37 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'HELLX9', 'HELL', '599'
);

/* INSERT QUERY NO: 38 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'HFSK', 'HELL', '599'
);

/* INSERT QUERY NO: 39 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'ISCAT-A', 'ISCAT', '599'
);

/* INSERT QUERY NO: 40 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'ISCAT-B', 'ISCAT', '599'
);

/* INSERT QUERY NO: 41 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JS8', 'MFSK', '599'
);

/* INSERT QUERY NO: 42 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT4A', 'JT4', '599'
);

/* INSERT QUERY NO: 43 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT4B', 'JT4', '599'
);

/* INSERT QUERY NO: 44 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT4C', 'JT4', '599'
);

/* INSERT QUERY NO: 45 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT4D', 'JT4', '599'
);

/* INSERT QUERY NO: 46 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT4E', 'JT4', '599'
);

/* INSERT QUERY NO: 47 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT4F', 'JT4', '599'
);

/* INSERT QUERY NO: 48 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT4G', 'JT4', '599'
);

/* INSERT QUERY NO: 49 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9-1', 'JT9', '599'
);

/* INSERT QUERY NO: 50 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9-2', 'JT9', '599'
);

/* INSERT QUERY NO: 51 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9-5', 'JT9', '599'
);

/* INSERT QUERY NO: 52 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9-10', 'JT9', '599'
);

/* INSERT QUERY NO: 53 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9-30', 'JT9', '599'
);

/* INSERT QUERY NO: 54 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9A', 'JT9', '599'
);

/* INSERT QUERY NO: 55 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9B', 'JT9', '599'
);

/* INSERT QUERY NO: 56 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9C', 'JT9', '599'
);

/* INSERT QUERY NO: 57 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9D', 'JT9', '599'
);

/* INSERT QUERY NO: 58 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9E', 'JT9', '599'
);

/* INSERT QUERY NO: 59 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9E FAST', 'JT9', '599'
);

/* INSERT QUERY NO: 60 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9F', 'JT9', '599'
);

/* INSERT QUERY NO: 61 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9F FAST', 'JT9', '599'
);

/* INSERT QUERY NO: 62 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9G', 'JT9', '599'
);

/* INSERT QUERY NO: 63 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9G FAST', 'JT9', '599'
);

/* INSERT QUERY NO: 64 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9H', 'JT9', '599'
);

/* INSERT QUERY NO: 65 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT9H FAST', 'JT9', '599'
);

/* INSERT QUERY NO: 66 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT65A', 'JT65', '599'
);

/* INSERT QUERY NO: 67 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT65B', 'JT65', '599'
);

/* INSERT QUERY NO: 68 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT65B2', 'JT65', '599'
);

/* INSERT QUERY NO: 69 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT65C', 'JT65', '599'
);

/* INSERT QUERY NO: 70 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JT65C2', 'JT65', '599'
);

/* INSERT QUERY NO: 71 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'JTMS', 'MFSK', '599'
);

/* INSERT QUERY NO: 72 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'LSB', 'SSB', '59'
);

/* INSERT QUERY NO: 73 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'MFSK4', 'MFSK', '599'
);

/* INSERT QUERY NO: 74 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'MFSK8', 'MFSK', '599'
);

/* INSERT QUERY NO: 75 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'MFSK11', 'MFSK', '599'
);

/* INSERT QUERY NO: 76 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'MFSK16', 'MFSK', '599'
);

/* INSERT QUERY NO: 77 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'MFSK22', 'MFSK', '599'
);

/* INSERT QUERY NO: 78 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'MFSK31', 'MFSK', '599'
);

/* INSERT QUERY NO: 79 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'MFSK32', 'MFSK', '599'
);

/* INSERT QUERY NO: 80 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'MFSK64', 'MFSK', '599'
);

/* INSERT QUERY NO: 81 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'MFSK64L', 'MFSK', '599'
);

/* INSERT QUERY NO: 82 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'MFSK128', 'MFSK', '599'
);

/* INSERT QUERY NO: 83 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'MFSK128L', 'MFSK', '599'
);

/* INSERT QUERY NO: 84 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'NAVTEX', 'TOR', '599'
);

/* INSERT QUERY NO: 85 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'OLIVIA 4/125', 'OLIVIA', '599'
);

/* INSERT QUERY NO: 86 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'OLIVIA 4/250', 'OLIVIA', '599'
);

/* INSERT QUERY NO: 87 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'OLIVIA 8/250', 'OLIVIA', '599'
);

/* INSERT QUERY NO: 88 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'OLIVIA 8/500', 'OLIVIA', '599'
);

/* INSERT QUERY NO: 89 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'OLIVIA 16/500', 'OLIVIA', '599'
);

/* INSERT QUERY NO: 90 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'OLIVIA 16/1000', 'OLIVIA', '599'
);

/* INSERT QUERY NO: 91 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'OLIVIA 32/1000', 'OLIVIA', '599'
);

/* INSERT QUERY NO: 92 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'OPERA-BEACON', 'OPERA', '599'
);

/* INSERT QUERY NO: 93 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'OPERA-QSO', 'OPERA', '599'
);

/* INSERT QUERY NO: 94 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PAC2', 'PAC', '599'
);

/* INSERT QUERY NO: 95 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PAC3', 'PAC', '599'
);

/* INSERT QUERY NO: 96 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PAC4', 'PAC', '599'
);

/* INSERT QUERY NO: 97 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PAX2', 'PAX', '599'
);

/* INSERT QUERY NO: 98 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PCW', 'CW', '599'
);

/* INSERT QUERY NO: 99 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK10', 'PSK', '599'
);

/* INSERT QUERY NO: 100 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK31', 'PSK', '599'
);

/* INSERT QUERY NO: 101 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK63', 'PSK', '599'
);

/* INSERT QUERY NO: 102 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK63F', 'PSK', '599'
);

/* INSERT QUERY NO: 103 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK63RC10', 'PSK', '599'
);

/* INSERT QUERY NO: 104 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK63RC20', 'PSK', '599'
);

/* INSERT QUERY NO: 105 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK63RC32', 'PSK', '599'
);

/* INSERT QUERY NO: 106 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK63RC4', 'PSK', '599'
);

/* INSERT QUERY NO: 107 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK63RC5', 'PSK', '599'
);

/* INSERT QUERY NO: 108 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK125', 'PSK', '599'
);

/* INSERT QUERY NO: 109 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK125RC10', 'PSK', '599'
);

/* INSERT QUERY NO: 110 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK125RC12', 'PSK', '599'
);

/* INSERT QUERY NO: 111 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK125RC16', 'PSK', '599'
);

/* INSERT QUERY NO: 112 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK125RC4', 'PSK', '599'
);

/* INSERT QUERY NO: 113 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK125RC5', 'PSK', '599'
);

/* INSERT QUERY NO: 114 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK250', 'PSK', '599'
);

/* INSERT QUERY NO: 115 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK250RC2', 'PSK', '599'
);

/* INSERT QUERY NO: 116 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK250RC3', 'PSK', '599'
);

/* INSERT QUERY NO: 117 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK250RC5', 'PSK', '599'
);

/* INSERT QUERY NO: 118 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK250RC6', 'PSK', '599'
);

/* INSERT QUERY NO: 119 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK250RC7', 'PSK', '599'
);

/* INSERT QUERY NO: 120 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK500', 'PSK', '599'
);

/* INSERT QUERY NO: 121 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK500RC2', 'PSK', '599'
);

/* INSERT QUERY NO: 122 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK500RC3', 'PSK', '599'
);

/* INSERT QUERY NO: 123 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK500RC4', 'PSK', '599'
);

/* INSERT QUERY NO: 124 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK800RC2', 'PSK', '599'
);

/* INSERT QUERY NO: 125 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK1000', 'PSK', '599'
);

/* INSERT QUERY NO: 126 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSK1000RC2', 'PSK', '599'
);

/* INSERT QUERY NO: 127 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSKAM10', 'PSK', '599'
);

/* INSERT QUERY NO: 128 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSKAM31', 'PSK', '599'
);

/* INSERT QUERY NO: 129 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSKAM50', 'PSK', '599'
);

/* INSERT QUERY NO: 130 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSKFEC31', 'PSK', '599'
);

/* INSERT QUERY NO: 131 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'PSKHELL', 'HELL', '599'
);

/* INSERT QUERY NO: 132 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'QPSK31', 'PSK', '599'
);

/* INSERT QUERY NO: 133 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'Q65', 'MFSK', '599'
);

/* INSERT QUERY NO: 134 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'QPSK63', 'PSK', '599'
);

/* INSERT QUERY NO: 135 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'QPSK125', 'PSK', '599'
);

/* INSERT QUERY NO: 136 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'QPSK250', 'PSK', '599'
);

/* INSERT QUERY NO: 137 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'QPSK500', 'PSK', '599'
);

/* INSERT QUERY NO: 138 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'QRA64A', 'QRA64', '599'
);

/* INSERT QUERY NO: 139 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'QRA64B', 'QRA64', '599'
);

/* INSERT QUERY NO: 140 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'QRA64C', 'QRA64', '599'
);

/* INSERT QUERY NO: 141 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'QRA64D', 'QRA64', '599'
);

/* INSERT QUERY NO: 142 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'QRA64E', 'QRA64', '599'
);

/* INSERT QUERY NO: 143 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'ROS-EME', 'ROS', '599'
);

/* INSERT QUERY NO: 144 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'ROS-HF', 'ROS', '599'
);

/* INSERT QUERY NO: 145 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'ROS-MF', 'ROS', '599'
);

/* INSERT QUERY NO: 146 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'SIM31', 'PSK', '599'
);

/* INSERT QUERY NO: 147 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'SITORB', 'TOR', '599'
);

/* INSERT QUERY NO: 148 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'SLOWHELL', 'HELL', '599'
);

/* INSERT QUERY NO: 149 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THOR-M', 'THOR', '599'
);

/* INSERT QUERY NO: 150 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THOR4', 'THOR', '599'
);

/* INSERT QUERY NO: 151 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THOR5', 'THOR', '599'
);

/* INSERT QUERY NO: 152 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THOR8', 'THOR', '599'
);

/* INSERT QUERY NO: 153 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THOR11', 'THOR', '599'
);

/* INSERT QUERY NO: 154 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THOR16', 'THOR', '599'
);

/* INSERT QUERY NO: 155 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THOR22', 'THOR', '599'
);

/* INSERT QUERY NO: 156 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THOR25X4', 'THOR', '599'
);

/* INSERT QUERY NO: 157 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THOR50X1', 'THOR', '599'
);

/* INSERT QUERY NO: 158 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THOR50X2', 'THOR', '599'
);

/* INSERT QUERY NO: 159 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THOR100', 'THOR', '599'
);

/* INSERT QUERY NO: 160 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THRBX', 'THRB', '599'
);

/* INSERT QUERY NO: 161 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THRBX1', 'THRB', '599'
);

/* INSERT QUERY NO: 162 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THRBX2', 'THRB', '599'
);

/* INSERT QUERY NO: 163 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THRBX4', 'THRB', '599'
);

/* INSERT QUERY NO: 164 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THROB1', 'THRB', '599'
);

/* INSERT QUERY NO: 165 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THROB2', 'THRB', '599'
);

/* INSERT QUERY NO: 166 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'THROB4', 'THRB', '599'
);

/* INSERT QUERY NO: 167 */
INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'USB', 'SSB', '59'
);

INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'FM', 'FM', '59'
);

INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'AM', 'AM', '59'
);

INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'CW', 'CW', '599'
);

INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'RTTYM', 'RTTYM', '599'
);

INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'SSTV', 'SSTV', '599'
);

INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'WSPR', 'WSPR', '599'
);

INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'MT63', 'MT63', '599'
);

INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'MSK144', 'MSK144', '599'
);

INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'FT8', 'FT8', '599'
);

INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'FSK441', 'FSK441', '599'
);

INSERT INTO mode(name, super_mode, default_rst)
VALUES
(
'C4FM', 'C4FM', '59'
);


COMMIT;
