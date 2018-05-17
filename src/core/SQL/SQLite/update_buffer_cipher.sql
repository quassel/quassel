UPDATE buffer
SET cipher = :cipher
WHERE userid = :userid AND networkid = :networkid AND buffercname = :buffercname
