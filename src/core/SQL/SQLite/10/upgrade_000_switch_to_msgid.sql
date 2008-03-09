UPDATE buffer
SET lastseen = (SELECT messageid FROM backlog WHERE backlog.bufferid = bufferid AND backlog.time = lastseen LIMIT 1)
WHERE lastseen != 0
