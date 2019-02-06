SELECT messageid, time,  type, flags, sender, senderprefixes, realname, avatarurl, message
FROM backlog
JOIN sender ON backlog.senderid = sender.senderid
WHERE backlog.messageid >= :first
    AND backlog.messageid <= (SELECT buffer.lastmsgid FROM buffer WHERE buffer.bufferid = :bufferDup1)
    AND bufferid = :buffer
    AND backlog.type & :type != 0
    AND (:flags = 0 OR backlog.flags & :flagsDup1 != 0)
ORDER BY messageid DESC
LIMIT :limit
