UPDATE buffer
SET buffername = $1, buffercname = $2
WHERE userid = $3 AND bufferid = $4
