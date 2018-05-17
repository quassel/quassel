CREATE TABLE sender ( -- THE SENDER OF IRC MESSAGES
       senderid INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
       sender TEXT NOT NULL,
       realname TEXT,
       avatarurl TEXT
);
