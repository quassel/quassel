SELECT bufferid, networkid, buffertype, groupid, buffername
FROM buffer
WHERE bufferid = :bufferid AND userid = :userid
