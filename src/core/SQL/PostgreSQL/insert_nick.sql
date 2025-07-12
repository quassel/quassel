INSERT INTO identity_nick (identityid, nick)
VALUES (:identityid, :nick)
ON CONFLICT (identityid, nick) DO NOTHING
