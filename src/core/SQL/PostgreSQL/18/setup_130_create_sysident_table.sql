CREATE TABLE sysident (
       sysidentid serial NOT NULL PRIMARY KEY,
       userid INTEGER NOT NULL,
       sysident TEXT NOT NULL,
       UNIQUE (userid, sysident)
)
