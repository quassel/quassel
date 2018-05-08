SELECT buffername, cipher
FROM buffer
WHERE userid = :userid AND networkid = :networkid AND buffertype IN (2, 4)
