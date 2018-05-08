SELECT COALESCE(t.sum,0)
FROM
  (SELECT COUNT(*) AS sum FROM backlog
   WHERE bufferid = :bufferid
     AND flags & 2 != 0
     AND flags & 1 = 0
     AND messageid > :lastseenmsgid) t;
