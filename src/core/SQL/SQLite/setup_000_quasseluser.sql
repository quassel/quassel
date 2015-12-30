CREATE TABLE quasseluser (
       userid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
       username TEXT UNIQUE NOT NULL,
       password TEXT NOT NULL,
       hashversion INTEGER NOT NULL DEFAULT 0,
       authenticator varchar(64) NOT NULL DEFAULT "Database"
)
