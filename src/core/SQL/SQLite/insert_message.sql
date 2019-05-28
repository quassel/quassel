INSERT INTO backlog (time, bufferid, type, flags, ignored, senderid, senderprefixes, message)
VALUES (:time, :bufferid, :type, :flags,
    :ignored,
	(SELECT senderid FROM sender WHERE sender = :sender AND coalesce(realname, '') = coalesce(:realname, '') AND coalesce(avatarurl, '') = coalesce(:avatarurl, '')),
	:senderprefixes, :message
)
