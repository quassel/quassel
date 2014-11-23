UPDATE network
SET awaymessage = $1
WHERE userid = $2 AND networkid = $3
