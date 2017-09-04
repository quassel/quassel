UPDATE buffer
SET lastseenmsgid = min(:lastseenmsgid, buffer.lastmsgid)
WHERE userid = :userid AND bufferid = :bufferid
