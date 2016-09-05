CREATE TABLE ircserver (
    serverid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
    userid INTEGER NOT NULL,
    networkid INTEGER NOT NULL,
    hostname TEXT NOT NULL,
    port INTEGER NOT NULL DEFAULT 6667,
    password TEXT,
    ssl INTEGER NOT NULL DEFAULT 0, -- bool
    sslversion INTEGER NOT NULL DEFAULT 0,
    useproxy INTEGER NOT NULL DEFAULT 0, -- bool
    proxytype INTEGER NOT NULL DEFAULT 0,
    proxyhost TEXT NOT NULL DEFAULT 'localhost',
    proxyport INTEGER NOT NULL DEFAULT 8080,
    proxyuser TEXT,
    proxypass TEXT,
    sslverify INTEGER NOT NULL DEFAULT 0 -- bool, validate SSL cert
)
