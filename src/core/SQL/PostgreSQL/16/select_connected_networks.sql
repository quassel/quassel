SELECT networkid
FROM network
WHERE userid = $1 AND connected = true
