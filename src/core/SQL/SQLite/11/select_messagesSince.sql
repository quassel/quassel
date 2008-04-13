SELECT messageid, time,  type, flags, sender, message
FROM backlog
JOIN sender ON backlog.senderid = sender.senderid
WHERE bufferid = :bufferid AND time >= :since
ORDER BY messageid DESC
LIMIT -1 OFFSET :offset
