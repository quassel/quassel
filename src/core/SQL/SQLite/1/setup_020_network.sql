CREATE TABLE network (
       networkid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
       userid INTEGER NOT NULL,
       networkname TEXT NOT NULL,
       UNIQUE (userid, networkname))
