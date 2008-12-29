CREATE TABLE identity (
       identityid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
       userid INTEGER NOT NULL,
       identityname TEXT NOT NULL,
       realname TEXT NOT NULL,
       awaynick TEXT,
       awaynickenabled INTEGER NOT NULL DEFAULT 0, -- BOOL
       awayreason TEXT,
       awayreasonenabled INTEGER NOT NULL DEFAULT 0, -- BOOL
       autoawayenabled INTEGER NOT NULL DEFAULT 0, -- BOOL
       autoawaytime INTEGER NOT NULL,
       autoawayreason TEXT,
       autoawayreasonenabled INTEGER NOT NULL DEFAULT 0, -- BOOL
       detachawayenabled INTEGER NOT NULL DEFAULT 0, -- BOOL
       detachawayreason TEXT,
       detachawayreasonenabled INTEGER NOT NULL DEFAULT 0, -- BOOL       
       ident TEXT,
       kickreason TEXT,
       partreason TEXT,
       quitreason TEXT,
       sslcert BLOB,
       sslkey BLOB,
       UNIQUE (userid, identityname)
)
