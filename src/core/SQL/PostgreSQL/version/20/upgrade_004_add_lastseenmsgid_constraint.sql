ALTER TABLE buffer ADD CONSTRAINT badLastSeenMsgId CHECK (buffer.lastseenmsgid <= buffer.lastmsgid)
