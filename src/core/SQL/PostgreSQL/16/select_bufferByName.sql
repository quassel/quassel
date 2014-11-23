SELECT bufferid, buffertype, groupid
FROM buffer
WHERE networkid = $1 AND userid = $2 AND buffercname = $3
