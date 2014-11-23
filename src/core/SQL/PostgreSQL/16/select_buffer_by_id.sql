SELECT bufferid, networkid, buffertype, groupid, buffername
FROM buffer
WHERE userid = $1 AND bufferid = $2
