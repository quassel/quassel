ALTER TABLE network
ADD COLUMN usesasl boolean NOT NULL DEFAULT FALSE,
ADD COLUMN saslaccount varchar(128),
ADD COLUMN saslpassword varchar(128)
