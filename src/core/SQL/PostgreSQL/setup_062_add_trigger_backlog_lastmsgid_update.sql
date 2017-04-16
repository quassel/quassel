CREATE TRIGGER backlog_lastmsgid_update_trigger
AFTER INSERT OR UPDATE
ON public.backlog
FOR EACH ROW
EXECUTE PROCEDURE public.backlog_lastmsgid_update();
