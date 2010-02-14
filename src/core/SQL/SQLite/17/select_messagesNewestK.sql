SELECT messageid, time,  type, flags, sender, message
FROM backlog
JOIN sender ON backlog.senderid = sender.senderid
WHERE bufferid = :bufferid
ORDER BY messageid DESC
LIMIT :limit
