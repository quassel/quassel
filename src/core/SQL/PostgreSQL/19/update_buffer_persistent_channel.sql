UPDATE buffer
SET joined = :joined
WHERE userid = :userid AND networkid = :networkid AND buffercname = :buffercname AND buffertype = 2
