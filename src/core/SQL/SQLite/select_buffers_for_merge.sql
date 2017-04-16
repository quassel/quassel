SELECT count(bufferid)
FROM buffer
WHERE (bufferid = :oldbufferid OR bufferid = :newbufferid) AND userid = :userid
