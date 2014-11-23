INSERT INTO buffer (userid, networkid, buffername, buffercname, buffertype, joined)
VALUES ($1, $2, $3, $4, $5, $6)
RETURNING bufferid
