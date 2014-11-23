SELECT nick
FROM identity_nick
WHERE identityid = $1
ORDER BY nickid ASC
