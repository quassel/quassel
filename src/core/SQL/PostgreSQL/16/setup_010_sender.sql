CREATE TABLE sender ( -- THE SENDER OF IRC MESSAGES
       senderid serial NOT NULL PRIMARY KEY,
       sender varchar(255) UNIQUE NOT NULL
)
