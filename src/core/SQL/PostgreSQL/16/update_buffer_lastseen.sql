UPDATE buffer
SET lastseenmsgid = $1
WHERE userid = $2 AND bufferid = $3
