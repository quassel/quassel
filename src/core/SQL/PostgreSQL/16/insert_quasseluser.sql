INSERT INTO quasseluser (username, password)
VALUES ($1, $2)
RETURNING userid
