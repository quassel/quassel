SELECT networkid
FROM network
WHERE userid = :userid AND connected = true
