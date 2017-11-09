CREATE OR REPLACE FUNCTION populate_lastmsgid() RETURNS void AS $$
DECLARE
	i buffer%rowtype;
BEGIN
	FOR i IN SELECT * FROM buffer
	LOOP
		UPDATE buffer
			SET lastmsgid = COALESCE((
				SELECT backlog.messageid
				FROM backlog
				WHERE backlog.bufferid = i.bufferid
				ORDER BY messageid DESC LIMIT 1
			), 0)
			WHERE buffer.bufferid = i.bufferid;
	END LOOP;
	RETURN;
END
$$ LANGUAGE plpgsql;
