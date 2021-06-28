BEGIN TRANSACTION;


CREATE TABLE station (
	name TEXT,
	CALL TEXT,
	QTH TEXT,
	QRA TEXT,
	ASL TEXT,
	rig TEXT,
	ant TEXT
);

CREATE TABLE mode (
	name TEXT,
	default_rst TEXT default "599",
	comment TEXT default ""
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
	comment TEXT,
	station INTEGER
);

INSERT INTO "mode" VALUES ('CW','599',NULL);
INSERT INTO "mode" VALUES ('PSK31','599',NULL);
INSERT INTO "mode" VALUES ('PSK63','599',NULL);
INSERT INTO "mode" VALUES ('MFSK16','599',NULL);
INSERT INTO "mode" VALUES ('THOR11','599',NULL);
INSERT INTO "mode" VALUES ('FT8','599',NULL);
INSERT INTO "mode" VALUES ('LSB','59',NULL);
INSERT INTO "mode" VALUES ('USB','59',NULL);
INSERT INTO "mode" VALUES ('FM','59',NULL);


COMMIT;
