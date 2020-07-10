SELECT messageid, time, type, flags, sender, senderprefixes, realname, avatarurl, message
FROM backlog
JOIN sender ON backlog.senderid = sender.senderid
WHERE backlog.messageid >= :firstmsg
    AND backlog.messageid <= (SELECT buffer.lastmsgid FROM buffer WHERE buffer.bufferid = :bufferid)
    AND bufferid = :bufferid
    AND (:type <= 0 OR backlog.type & :type != 0)
    AND (:flags <= 0 OR backlog.flags & :flags != 0)
ORDER BY messageid DESC
LIMIT :limit
