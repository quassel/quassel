CREATE TABLE quasseluser (
       userid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
       username TEXT UNIQUE NOT NULL,
       password TEXT NOT NULL,
       hashversion INTEGER NOT NULL DEFAULT 0,
       authenticator TEXT NOT NULL DEFAULT "Database",
       outgoing_ip TEXT NOT NULL DEFAULT ""
)
