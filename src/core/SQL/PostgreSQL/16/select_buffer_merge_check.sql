SELECT count(*) FROM buffer
WHERE userid = $1 AND bufferid IN ($2, $3)