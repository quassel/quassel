SELECT messageid, time, type, flags, sender, senderprefixes, realname, avatarurl, message
FROM backlog
JOIN sender ON backlog.senderid = sender.senderid
WHERE backlog.messageid >= $1
    AND backlog.messageid < $2
    AND bufferid = $3
    AND ($4 <= 0 OR backlog.type & $4 != 0)
    AND ($5 <= 0 OR backlog.flags & $5 != 0)
ORDER BY messageid ASC
LIMIT $6
