INSERT INTO backlog (time, bufferid, type, flags, senderid, message)
VALUES (:time, :bufferid, :type, :flags, (SELECT senderid FROM sender WHERE sender = :sender), :message)
