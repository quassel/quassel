SELECT count(*)
FROM backlog
WHERE bufferid = :bufferid AND time >= :since
