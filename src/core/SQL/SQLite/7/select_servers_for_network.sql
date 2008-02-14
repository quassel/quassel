SELECT hostname, port, password, ssl
FROM ircserver
WHERE networkid = :networkid
