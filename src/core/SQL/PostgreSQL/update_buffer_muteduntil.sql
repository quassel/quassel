UPDATE buffer
SET muteduntil = :muteduntil
WHERE userid = :userid AND bufferid = :bufferid
