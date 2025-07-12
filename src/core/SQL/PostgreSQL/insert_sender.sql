INSERT INTO sender (sender, realname, avatarurl)
VALUES ($1, $2, $3)
ON CONFLICT (sender, realname, avatarurl) DO NOTHING
RETURNING senderid
