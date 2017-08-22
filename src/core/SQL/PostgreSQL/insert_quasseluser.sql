INSERT INTO quasseluser (username, password, hashversion, authenticator)
VALUES (:username, :password, :hashversion, :authenticator)
RETURNING userid
