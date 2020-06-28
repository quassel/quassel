SELECT messageid, time,  type, flags, sender, senderprefixes, realname, avatarurl, message
FROM backlog
JOIN sender ON backlog.senderid = sender.senderid
WHERE bufferid = :bufferid
    AND backlog.messageid >= :firstmsg
    AND backlog.messageid < :lastmsg
    AND (:type = 0 OR backlog.type & :type != 0)
    AND (:flags = 0 OR backlog.flags & :flags != 0)
ORDER BY messageid ASC
LIMIT :limit
