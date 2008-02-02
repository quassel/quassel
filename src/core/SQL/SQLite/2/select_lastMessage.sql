SELECT messageid FROM backlog
WHERE time = :time AND bufferid = :bufferid AND type = :type AND senderid = (SELECT senderid FROM sender WHERE sender = :sender)
