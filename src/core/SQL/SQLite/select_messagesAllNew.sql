SELECT messageid, bufferid, time,  type, flags, sender, senderprefixes, message
FROM backlog
JOIN sender ON backlog.senderid = sender.senderid
WHERE backlog.bufferid IN (SELECT bufferid FROM buffer WHERE userid = :userid)
    AND backlog.messageid >= :firstmsg
ORDER BY messageid DESC
LIMIT :limit
