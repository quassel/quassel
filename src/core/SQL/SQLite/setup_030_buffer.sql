CREATE TABLE buffer (
	bufferid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	userid INTEGER NOT NULL,
	groupid INTEGER,
	networkid INTEGER NOT NULL,
	buffername TEXT NOT NULL,
	buffercname TEXT NOT NULL, -- CANONICAL BUFFER NAME (lowercase version)
	buffertype INTEGER NOT NULL DEFAULT 0,
	lastmsgid INTEGER NOT NULL DEFAULT 0,
	lastseenmsgid INTEGER NOT NULL DEFAULT 0,
	markerlinemsgid INTEGER NOT NULL DEFAULT 0,
	bufferactivity INTEGER NOT NULL DEFAULT 0,
	key TEXT,
	joined INTEGER NOT NULL DEFAULT 0, -- BOOL
	cipher TEXT,
	CHECK (lastseenmsgid <= lastmsgid)
)
