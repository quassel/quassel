INSERT INTO buffer (userid, networkid, buffername, buffercname, buffertype, joined)
VALUES (:userid, :networkid, :buffername, :buffercname, :buffertype, :joined)
RETURNING bufferid
