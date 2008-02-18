UPDATE buffer
SET lastseen = :lastseen
WHERE userid = :userid AND bufferid = :bufferid
