INSERT INTO buffer (userid, networkid, buffername, buffercname, buffertype, joined)
VALUES (:userid, :networkid, :buffername, :buffercname, :buffertype, :joined)
ON CONFLICT (userid, networkid, buffercname) DO NOTHING
RETURNING bufferid
