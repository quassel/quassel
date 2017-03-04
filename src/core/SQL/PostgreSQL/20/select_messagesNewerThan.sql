SELECT messageid, time,  type, flags, sender, message
FROM backlog
JOIN sender ON backlog.senderid = sender.senderid
WHERE backlog.messageid >= $1
AND backlog.messageid <= (SELECT buffer.lastmsgid FROM buffer WHERE buffer.bufferid = $1)
AND bufferid = $2
ORDER BY messageid DESC
LIMIT $3
