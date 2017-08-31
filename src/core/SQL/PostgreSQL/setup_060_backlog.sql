CREATE TABLE backlog (
	messageid serial PRIMARY KEY,
	time timestamp NOT NULL,
	bufferid integer NOT NULL REFERENCES buffer (bufferid) ON DELETE CASCADE,
	type integer NOT NULL,
	flags integer NOT NULL,
	senderid integer NOT NULL REFERENCES sender (senderid) ON DELETE SET NULL,
    senderprefixes TEXT,
	message TEXT
)
