SELECT buffername, key
FROM buffer
WHERE userid = :userid AND networkid = :networkid AND buffertype = 2 AND joined = 1
