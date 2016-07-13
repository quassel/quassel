SELECT hostname, port, password, ssl, sslversion, useproxy, proxytype, proxyhost, proxyport, proxyuser, proxypass, sslverify
FROM ircserver
WHERE networkid = :networkid
