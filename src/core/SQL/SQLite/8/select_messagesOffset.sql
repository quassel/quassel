SELECT count(*)
FROM backlog
WHERE bufferid = :bufferid AND messageid < :messageid
