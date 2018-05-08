SELECT messageid, time,  type, flags, sender, senderprefixes, realname, avatarurl, message
FROM backlog
JOIN sender ON backlog.senderid = sender.senderid
WHERE backlog.messageid >= $1
    AND backlog.messageid < $2
    AND bufferid = $3
ORDER BY messageid DESC
LIMIT $4
