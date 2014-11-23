SELECT count(*)
FROM identity
WHERE identityid = $1 AND userid = $2
