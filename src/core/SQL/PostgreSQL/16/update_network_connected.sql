UPDATE network
SET connected = $1
WHERE userid = $2 AND networkid = $3
