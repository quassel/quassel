CREATE TABLE network (
       networkid serial NOT NULL PRIMARY KEY,
       userid integer NOT NULL REFERENCES quasseluser (userid) ON DELETE CASCADE,
       networkname varchar(32) NOT NULL,
       identityid integer REFERENCES identity (identityid) ON DELETE SET NULL,
       encodingcodec varchar(32) NOT NULL DEFAULT 'ISO-8859-15',
       decodingcodec varchar(32) NOT NULL DEFAULT 'ISO-8859-15',
       servercodec varchar(32),
       userandomserver boolean NOT NULL DEFAULT FALSE,
       perform TEXT,
       useautoidentify boolean NOT NULL DEFAULT FALSE,
       autoidentifyservice varchar(128),
       autoidentifypassword varchar(128),
       usesasl boolean NOT NULL DEFAULT FALSE,
       saslaccount varchar(128),
       saslpassword varchar(128),
       useautoreconnect boolean NOT NULL DEFAULT TRUE,
       autoreconnectinterval integer NOT NULL DEFAULT 0,
       autoreconnectretries integer NOT NULL DEFAULT 0,
       unlimitedconnectretries boolean NOT NULL DEFAULT FALSE,
       rejoinchannels boolean NOT NULL DEFAULT FALSE,
       connected boolean NOT NULL DEFAULT FALSE,
       usermode varchar(32), -- user mode to restore
       awaymessage varchar(256), -- away message to restore (empty if not away)
       attachperform text, -- perform list for on attach
       detachperform text, -- perform list for on detach
       usecustomessagerate boolean NOT NULL DEFAULT FALSE,  -- Custom rate limiting
       messagerateburstsize INTEGER NOT NULL DEFAULT 5,     -- Maximum messages at once
       messageratedelay INTEGER NOT NULL DEFAULT 2200,      -- Delay between future messages (milliseconds)
       unlimitedmessagerate boolean NOT NULL DEFAULT FALSE, -- Disable rate limits
       UNIQUE (userid, networkname)
)
