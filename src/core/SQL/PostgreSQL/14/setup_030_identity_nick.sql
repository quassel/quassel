CREATE TABLE identity_nick (
       nickid serial PRIMARY KEY,
       identityid integer NOT NULL REFERENCES identity (identityid) ON DELETE CASCADE,
       nick varchar(64) NOT NULL,
       UNIQUE (identityid, nick)
)
