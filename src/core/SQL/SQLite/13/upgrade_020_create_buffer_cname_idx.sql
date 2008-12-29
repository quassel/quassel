CREATE UNIQUE INDEX IF NOT EXISTS buffer_idx
       ON buffer(userid, networkid, buffername)
