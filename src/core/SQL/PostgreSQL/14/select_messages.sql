SELECT messageid, time,  type, flags, sender, message
FROM backlog
JOIN sender ON backlog.senderid = sender.senderid
WHERE bufferid = $1
ORDER BY messageid DESC
LIMIT $2