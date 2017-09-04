ALTER TABLE quasseluser
ADD COLUMN authenticator varchar(64) NOT NULL DEFAULT 'Database';
