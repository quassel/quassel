CREATE TABLE identity_nick (
       nickid serial PRIMARY KEY,
       identityid integer NOT NULL REFERENCES identity (identityid) ON DELETE CASCADE,
       nick TEXT NOT NULL,
       UNIQUE (identityid, nick)
)
