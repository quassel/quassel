CREATE TABLE identity_nick (
       nickid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
       identityid INTEGER NOT NULL,
       nick TEXT NOT NULL,
       UNIQUE (identityid, nick)
)
