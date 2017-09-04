UPDATE buffer
SET lastseenmsgid = least(:lastseenmsgid, buffer.lastmsgid)
WHERE userid = :userid AND bufferid = :bufferid
