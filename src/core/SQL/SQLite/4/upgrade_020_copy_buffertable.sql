INSERT INTO buffer (bufferid, userid, groupid, networkid, buffername, buffercname)
SELECT bufferid, userid, groupid, networkid, buffername, lower(buffername) FROM bufferold
