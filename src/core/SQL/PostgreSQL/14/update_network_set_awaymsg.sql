UPDATE network
SET awaymessage = :awaymsg
WHERE userid = :userid AND networkid = :networkid
