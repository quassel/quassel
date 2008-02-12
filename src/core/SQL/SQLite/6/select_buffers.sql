SELECT DISTINCT buffer.bufferid, network.networkid, buffertype, groupid, buffername
FROM buffer
JOIN network ON buffer.networkid = network.networkid
JOIN backlog ON buffer.bufferid = backlog.bufferid
WHERE buffer.userid = :userid AND time >= :time
