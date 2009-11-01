INSERT INTO backlog (time, bufferid, type, flags, senderid, message)
VALUES ($1, $2, $3, $4, (SELECT senderid FROM sender WHERE sender = $5), $6)
RETURNING messageid
