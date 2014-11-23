SELECT messageid, bufferid, time,  type, flags, sender, message
FROM backlog
JOIN sender ON backlog.senderid = sender.senderid
WHERE backlog.bufferid IN (SELECT bufferid FROM buffer WHERE userid = $1)
    AND backlog.messageid >= $2
ORDER BY messageid DESC
