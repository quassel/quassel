INSERT INTO backlog (time, bufferid, type, flags, senderid, senderprefixes, message)
VALUES (:time, :bufferid, :type, :flags, (SELECT senderid FROM sender WHERE sender = :sender), :senderprefixes, :message)
