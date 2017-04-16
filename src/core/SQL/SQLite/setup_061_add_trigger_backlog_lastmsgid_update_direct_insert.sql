CREATE TRIGGER IF NOT EXISTS backlog_lastmsgid_update_trigger_insert
AFTER INSERT
ON backlog
FOR EACH ROW
    BEGIN
        UPDATE buffer
        SET lastmsgid = new.messageid
        WHERE buffer.bufferid = new.bufferid
            AND buffer.lastmsgid < new.messageid;
    END
