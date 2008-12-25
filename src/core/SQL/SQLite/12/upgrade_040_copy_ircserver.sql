INSERT INTO ircserver (serverid, userid, networkid, hostname, port, password, ssl)
SELECT serverid, userid, networkid, hostname, port, password, ssl FROM ircserverold
