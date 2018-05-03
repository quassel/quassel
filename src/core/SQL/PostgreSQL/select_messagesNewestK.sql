SELECT messageid, time,  type, flags, sender, senderprefixes, realname, avatarurl, message
FROM backlog
JOIN sender ON backlog.senderid = sender.senderid
WHERE bufferid = $1
AND backlog.messageid <= (SELECT buffer.lastmsgid FROM buffer WHERE buffer.bufferid = $1)
ORDER BY messageid DESC
LIMIT $2
