INSERT INTO backlog (time, bufferid, type, flags, senderid, message)
VALUES ($1, $2, $3, $4, $5, $6)
RETURNING messageid
