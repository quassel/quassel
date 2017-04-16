INSERT INTO network (networkid, userid, networkname)
SELECT networkid, userid, networkname FROM networkold;
