UPDATE buffer
SET lastmsgid = (
	SELECT messageid 
	FROM backlog 
	WHERE backlog.bufferid = buffer.bufferid
	ORDER BY messageid 
	DESC LIMIT 1
);
