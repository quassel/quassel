CREATE TABLE quasseluser (
       userid serial NOT NULL PRIMARY KEY,
       username varchar(64) UNIQUE NOT NULL,
       password TEXT NOT NULL,
       hashversion integer NOT NULL DEFAULT 0,
       authenticator varchar(64) NOT NULL DEFAULT 'Database'
)
