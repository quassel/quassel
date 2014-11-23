SELECT bufferid, lastseenmsgid
FROM buffer
WHERE userid = $1
