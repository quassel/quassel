CREATE UNIQUE INDEX IF NOT EXISTS buffer_cname_idx
       ON buffer(userid, networkid, buffercname)
