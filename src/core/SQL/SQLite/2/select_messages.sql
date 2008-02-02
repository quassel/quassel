SELECT messageid, time,  type, flags, sender, message
FROM backlog
JOIN buffer ON backlog.bufferid = buffer.bufferid
JOIN sender ON backlog.senderid = sender.senderid
WHERE buffer.bufferid = :bufferid
ORDER BY messageid DESC
LIMIT :limit OFFSET :offset
