CREATE TABLE network (
       networkid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
       userid INTEGER NOT NULL,
       networkname TEXT NOT NULL,
       identityid INTEGER NOT NULL DEFAULT 1,
       encodingcodec TEXT NOT NULL DEFAULT "ISO-8859-15",
       decodingcodec TEXT NOT NULL DEFAULT "ISO-8859-15",
       servercodec TEXT NOT NULL DEFAULT "",
       userandomserver INTEGER NOT NULL DEFAULT 0, -- BOOL
       perform TEXT,
       useautoidentify INTEGER NOT NULL DEFAULT 0, -- BOOL
       autoidentifyservice TEXT,
       autoidentifypassword TEXT,
       useautoreconnect INTEGER NOT NULL DEFAULT 0, -- BOOL
       autoreconnectinterval INTEGER NOT NULL DEFAULT 0,
       autoreconnectretries INTEGER NOT NULL DEFAULT 0,
       unlimitedconnectretries INTEGER NOT NULL DEFAULT 0, -- BOOL
       rejoinchannels INTEGER NOT NULL DEFAULT 0, -- BOOL
       UNIQUE (userid, networkname)
)
