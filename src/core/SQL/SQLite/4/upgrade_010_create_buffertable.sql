CREATE TABLE buffer (
	bufferid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	userid INTEGER NOT NULL,
	groupid INTEGER,
	networkid INTEGER NOT NULL,
	buffername TEXT NOT NULL,
	buffercname TEXT NOT NULL -- CANONICAL BUFFER NAME (lowercase version)
)