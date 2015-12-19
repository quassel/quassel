CREATE TABLE sysident (
       sysidentid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
       userid INTEGER NOT NULL,
       sysident TEXT NOT NULL,
       UNIQUE (userid, sysident)
)
