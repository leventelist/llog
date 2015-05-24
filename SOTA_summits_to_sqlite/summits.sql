BEGIN TRANSACTION;

CREATE TABLE summits (SummitID INTEGER PRIMARY KEY AUTOINCREMENT,
SummitCode TEXT,
AssociationName TEXT,
RegionName TEXT,
SummitName TEXT,
AltM INTEGER,
AltFt INTEGER,
GridRef1 REAL,
GridRef2 REAL,
Longitude REAL,
Latitude REAL,
Points INTEGER,
BonusPoints INTEGER,
ValidFrom TEXT,
ValidTo TEXT,
ActivationCount INTEGER,
ActivationDate TEXT,
ActivationCall TEXT);

COMMIT;

