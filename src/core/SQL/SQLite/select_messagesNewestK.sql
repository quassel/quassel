SELECT messageid, time,  type, flags, sender, senderprefixes, realname, avatarurl, message
FROM backlog
JOIN sender ON backlog.senderid = sender.senderid
WHERE bufferid = :bufferid
AND backlog.messageid <= (SELECT buffer.lastmsgid FROM buffer WHERE buffer.bufferid = :bufferid)
ORDER BY messageid DESC
LIMIT :limit
