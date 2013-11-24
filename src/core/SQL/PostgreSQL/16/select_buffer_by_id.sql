SELECT bufferid, networkid, buffertype, groupid, buffername
FROM buffer
WHERE userid = :userid AND bufferid = :bufferid
