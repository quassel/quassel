ALTER TABLE quasseluser
ALTER COLUMN password TYPE text,
ADD COLUMN hashversion integer NOT NULL DEFAULT 0
