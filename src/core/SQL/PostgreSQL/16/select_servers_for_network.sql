SELECT hostname, port, password, ssl, sslversion, useproxy, proxytype, proxyhost, proxyport, proxyuser, proxypass
FROM ircserver
WHERE networkid = $1
