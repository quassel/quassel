INSERT INTO buffer (userid, networkid, buffername)
VALUES (:userid, (SELECT networkid FROM network WHERE networkname = :networkname AND userid = :userid2), :buffername)
