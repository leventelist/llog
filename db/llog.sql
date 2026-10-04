BEGIN TRANSACTION;


CREATE TABLE IF NOT EXISTS station (
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


CREATE TABLE IF NOT EXISTS log (
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

/*QSOs uploaded to online services, e.g. service = 'WRL'*/
CREATE TABLE IF NOT EXISTS upload (
	log_id INTEGER NOT NULL,
	service TEXT NOT NULL,
	remote_id TEXT,
	uploaded_at TEXT DEFAULT CURRENT_TIMESTAMP,
	UNIQUE(log_id, service)
);

/*Insert some (non)sensible data to the station table, unless it already has stations*/
INSERT INTO station(name, CALL) SELECT 'default', 'NOCALL' WHERE NOT EXISTS (SELECT 1 FROM station);

COMMIT;
