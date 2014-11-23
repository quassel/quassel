UPDATE buffer
SET joined = $1
WHERE userid = $2 AND networkid = $3 AND buffercname = $4 AND buffertype = 2
