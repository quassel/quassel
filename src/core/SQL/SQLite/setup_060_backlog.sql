CREATE TABLE backlog (
	messageid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	time INTEGER NOT NULL,
	bufferid INTEGER NOT NULL,
	type INTEGER NOT NULL,
	flags INTEGER NOT NULL,
    ignored boolean NOT NULL,
	senderid INTEGER NOT NULL,
	senderprefixes TEXT,
	message TEXT
)
