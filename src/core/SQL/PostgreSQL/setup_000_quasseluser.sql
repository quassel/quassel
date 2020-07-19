CREATE TABLE quasseluser (
       userid serial NOT NULL PRIMARY KEY,
       username TEXT UNIQUE NOT NULL,
       password TEXT NOT NULL,
       hashversion integer NOT NULL DEFAULT 0,
       authenticator TEXT NOT NULL DEFAULT 'Database'
)
