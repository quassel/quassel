UPDATE network
SET connected = :connected
WHERE userid = :userid AND networkid = :networkid
