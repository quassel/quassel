SELECT userid, password, hashversion
FROM quasseluser
WHERE username = :username
