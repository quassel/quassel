DELETE FROM backlog
WHERE bufferid IN (SELECT DISTINCT bufferid FROM buffer WHERE userid = $1)
