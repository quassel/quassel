INSERT INTO sender (sender, realname, avatarurl)
VALUES ($1, $2, $3)
RETURNING senderid
