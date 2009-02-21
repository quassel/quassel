SELECT bufferid, buffertype, groupid
FROM buffer
WHERE networkid = :networkid AND userid = :userid AND buffercname = :buffercname
