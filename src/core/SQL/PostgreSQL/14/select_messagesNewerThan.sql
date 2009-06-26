SELECT messageid, time,  type, flags, sender, message
FROM backlog
LEFT JOIN sender ON backlog.senderid = sender.senderid
WHERE backlog.messageid >= $1 AND bufferid = $2
ORDER BY messageid DESC
LIMIT $3