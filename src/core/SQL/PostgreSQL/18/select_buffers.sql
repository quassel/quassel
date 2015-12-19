SELECT DISTINCT buffer.bufferid, network.networkid, buffertype, groupid, buffername
FROM buffer
JOIN network ON buffer.networkid = network.networkid
WHERE buffer.userid = :userid
