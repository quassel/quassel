SELECT messageid, time, bufferid, type, flags, ignored, senderid, senderprefixes, message
FROM backlog
WHERE messageid > ? AND messageid <= ?
ORDER BY messageid ASC
