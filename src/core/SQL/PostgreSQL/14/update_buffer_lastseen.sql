UPDATE buffer
SET lastseenmsgid = :lastseenmsgid
WHERE userid = :userid AND bufferid = :bufferid
