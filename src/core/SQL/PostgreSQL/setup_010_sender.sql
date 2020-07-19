CREATE TABLE sender ( -- THE SENDER OF IRC MESSAGES
       senderid bigserial NOT NULL PRIMARY KEY,
       sender TEXT NOT NULL,
       realname TEXT,
       avatarurl TEXT
);
