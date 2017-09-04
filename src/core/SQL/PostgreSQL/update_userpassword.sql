UPDATE quasseluser
SET password = :password, hashversion = :hashversion
WHERE userid = :userid
