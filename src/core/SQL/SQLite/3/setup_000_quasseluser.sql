CREATE TABLE quasseluser (
       userid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
       username TEXT UNIQUE NOT NULL,
       password BLOB NOT NULL)

	  
