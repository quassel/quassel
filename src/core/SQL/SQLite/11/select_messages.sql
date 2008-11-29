SELECT messageid, time,  type, flags, sender, message
FROM backlog
JOIN buffer ON backlog.bufferid = buffer.bufferid
JOIN sender ON backlog.senderid = sender.senderid
WHERE buffer.bufferid = :bufferid
    AND backlog.messageid >= :firstmsg
    AND backlog.messageid < :lastmsg
ORDER BY messageid DESC
LIMIT :limit
