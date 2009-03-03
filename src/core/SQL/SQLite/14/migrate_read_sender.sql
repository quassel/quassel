SELECT senderid, sender
FROM sender
WHERE senderid > ? AND senderid <= ?
ORDER BY senderid ASC

