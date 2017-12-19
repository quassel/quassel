SELECT COALESCE(SUM(t.type),0)
FROM
  (SELECT DISTINCT TYPE
   FROM backlog
   WHERE bufferid = :bufferid
     AND messageid > :lastseenmsgid) t;
