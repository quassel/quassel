SELECT bufferid
FROM buffer
JOIN network ON buffer.networkid = network.networkid
WHERE network.networkname = :networkname AND network.userid = :userid AND buffer.userid = :userid2 AND lower(buffer.buffername) = lower(:buffername)
