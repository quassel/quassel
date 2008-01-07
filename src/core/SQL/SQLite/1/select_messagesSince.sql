SELECT messageid, time,  type, flags, sender, message, displayname
FROM backlog
JOIN buffer ON backlog.bufferid = buffer.bufferid
JOIN sender ON backlog.senderid = sender.senderid
LEFT JOIN buffergroup ON buffer.groupid = buffergroup.groupid
WHERE (buffer.bufferid = :bufferid OR buffer.groupid = (SELECT groupid FROM buffer WHERE bufferid = :bufferid2)) AND backlog.time >= :since
ORDER BY messageid DESC
LIMIT -1 OFFSET :offset
