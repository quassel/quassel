CREATE TABLE network (
       networkid serial NOT NULL PRIMARY KEY,
       userid integer NOT NULL REFERENCES quasseluser (userid) ON DELETE CASCADE,
       networkname TEXT NOT NULL,
       identityid integer REFERENCES identity (identityid) ON DELETE SET NULL,
       encodingcodec TEXT NOT NULL DEFAULT 'ISO-8859-15',
       decodingcodec TEXT NOT NULL DEFAULT 'ISO-8859-15',
       servercodec TEXT,
       userandomserver boolean NOT NULL DEFAULT FALSE,
       perform TEXT,
       useautoidentify boolean NOT NULL DEFAULT FALSE,
       autoidentifyservice TEXT,
       autoidentifypassword TEXT,
       usesasl boolean NOT NULL DEFAULT FALSE,
       saslaccount TEXT,
       saslpassword TEXT,
       useautoreconnect boolean NOT NULL DEFAULT TRUE,
       autoreconnectinterval integer NOT NULL DEFAULT 0,
       autoreconnectretries integer NOT NULL DEFAULT 0,
       unlimitedconnectretries boolean NOT NULL DEFAULT FALSE,
       rejoinchannels boolean NOT NULL DEFAULT FALSE,
       connected boolean NOT NULL DEFAULT FALSE,
       usermode TEXT, -- user mode to restore
       awaymessage TEXT, -- away message to restore (empty if not away)
       attachperform text, -- perform list for on attach
       detachperform text, -- perform list for on detach
       usecustomessagerate boolean NOT NULL DEFAULT FALSE,  -- Custom rate limiting
       messagerateburstsize INTEGER NOT NULL DEFAULT 5,     -- Maximum messages at once
       messageratedelay INTEGER NOT NULL DEFAULT 2200,      -- Delay between future messages (milliseconds)
       unlimitedmessagerate boolean NOT NULL DEFAULT FALSE, -- Disable rate limits
       skipcaps TEXT,                                       -- Space-separated IRCv3 caps to not auto-negotiate
       UNIQUE (userid, networkname)
)
