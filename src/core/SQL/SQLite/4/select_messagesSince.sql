SELECT messageid, time,  type, flags, sender, message
FROM backlog
JOIN buffer ON backlog.bufferid = buffer.bufferid
JOIN sender ON backlog.senderid = sender.senderid
WHERE buffer.bufferid = :bufferid AND backlog.time >= :since
ORDER BY messageid DESC
LIMIT -1 OFFSET :offset
