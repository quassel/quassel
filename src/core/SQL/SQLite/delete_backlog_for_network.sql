DELETE FROM backlog
WHERE bufferid IN (SELECT bufferid FROM buffer WHERE networkid = :networkid)
