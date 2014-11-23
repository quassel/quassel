UPDATE network
SET usermode = $1
WHERE userid = $2 AND networkid = $3