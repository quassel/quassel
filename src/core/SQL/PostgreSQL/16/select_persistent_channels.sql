SELECT buffername, key
FROM buffer
WHERE userid = $1 AND networkid = $2 AND buffertype = 2 AND joined = true
