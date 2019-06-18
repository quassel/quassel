CREATE TABLE sender ( -- THE SENDER OF IRC MESSAGES
       senderid bigserial NOT NULL PRIMARY KEY,
       sender varchar(8703) NOT NULL,
       realname TEXT,
       avatarurl TEXT
);
