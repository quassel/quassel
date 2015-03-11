CREATE TABLE quasseluser (
       userid serial NOT NULL PRIMARY KEY,
       username varchar(64) UNIQUE NOT NULL,
       password char(40) NOT NULL -- hex reppresentation of sha1 hashes
)
