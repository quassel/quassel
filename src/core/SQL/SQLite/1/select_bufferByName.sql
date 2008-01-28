SELECT bufferid
FROM buffer
WHERE buffer.networkid = :networkid AND buffer.userid = :userid AND lower(buffer.buffername) = lower(:buffername)
