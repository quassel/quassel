SELECT userid, password, hashversion, authenticator
FROM quasseluser
WHERE username = :username
