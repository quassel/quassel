CREATE OR REPLACE FUNCTION public.backlog_lastmsgid_update()
RETURNS trigger
AS $BODY$
    BEGIN
        UPDATE buffer
        SET lastmsgid = new.messageid
        WHERE buffer.bufferid = new.bufferid
            AND buffer.lastmsgid < new.messageid;
        RETURN new;
    END
$BODY$
LANGUAGE plpgsql;
