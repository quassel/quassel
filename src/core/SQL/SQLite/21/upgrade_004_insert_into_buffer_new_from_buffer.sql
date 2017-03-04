INSERT INTO buffer_new (
	bufferid, 
	userid, 
	groupid, 
	networkid, 
	buffername, 
	buffercname, 
	buffertype, 
	lastmsgid, 
	lastseenmsgid,
	markerlinemsgid,
	key,
	joined
)
SELECT 
	bufferid,
	userid,
	groupid,
	networkid,
	buffername,
	buffercname,
	buffertype,
	lastmsgid,
	lastseenmsgid,
	markerlinemsgid,
	key,
	joined
FROM buffer;
