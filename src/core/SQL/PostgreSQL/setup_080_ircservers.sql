CREATE TABLE ircserver (
    serverid serial PRIMARY KEY,
    userid integer NOT NULL REFERENCES quasseluser (userid) ON DELETE CASCADE,
    networkid integer NOT NULL REFERENCES network (networkid) ON DELETE CASCADE,
    hostname TEXT NOT NULL,
    port integer NOT NULL DEFAULT 6667,
    password TEXT,
    ssl boolean NOT NULL DEFAULT FALSE, -- bool
    sslversion integer NOT NULL DEFAULT 0,
    useproxy boolean NOT NULL DEFAULT FALSE, -- bool
    proxytype integer NOT NULL DEFAULT 0,
    proxyhost TEXT NOT NULL DEFAULT 'localhost',
    proxyport integer NOT NULL DEFAULT 8080,
    proxyuser TEXT,
    proxypass TEXT,
    sslverify boolean NOT NULL DEFAULT FALSE -- bool, validate SSL cert
)
