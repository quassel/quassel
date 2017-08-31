INSERT INTO backlog (time, bufferid, type, flags, senderid, senderprefixes, message)
VALUES ($1, $2, $3, $4, $5, $6, $7)
RETURNING messageid
