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
	POTA_REF TEXT,
	P2P_REF TEXT,
	WWFF_REF TEXT,
	W2W_REF TEXT,
	comment TEXT,
	station INTEGER default 1
);

/*Insert some (non)sensible data to the station table*/
INSERT INTO station(name, CALL) VALUES ("default", "NOCALL");

COMMIT;
