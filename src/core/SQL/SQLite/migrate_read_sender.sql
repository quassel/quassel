SELECT senderid, sender, realname, avatarurl
FROM sender
WHERE senderid > ? AND senderid <= ?
ORDER BY senderid ASC
