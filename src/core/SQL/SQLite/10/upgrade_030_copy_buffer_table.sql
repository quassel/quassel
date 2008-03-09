INSERT INTO buffer (bufferid, userid, groupid, networkid, buffername, buffercname, buffertype, lastseenmsgid, key, joined)
SELECT bufferid, userid, groupid, networkid, buffername, buffercname, buffertype, lastseen, key, joined FROM buffer_old
