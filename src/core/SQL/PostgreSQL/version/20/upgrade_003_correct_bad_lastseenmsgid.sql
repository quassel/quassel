UPDATE buffer
SET lastseenmsgid = buffer.lastmsgid
WHERE buffer.lastseenmsgid > buffer.lastmsgid
