SELECT bufferid, buffertype, groupid
FROM buffer
WHERE buffer.networkid = :networkid AND buffer.userid = :userid AND buffer.buffercname = :buffercname
