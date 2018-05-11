SELECT senderid
FROM sender
WHERE sender = $1 AND coalesce(realname, '') = coalesce($2, '') AND coalesce(avatarurl, '') = coalesce($3, '')
