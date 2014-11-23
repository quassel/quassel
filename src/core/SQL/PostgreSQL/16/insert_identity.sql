INSERT INTO identity (userid, identityname, realname, awaynick, awaynickenabled, awayreason, awayreasonenabled, autoawayenabled, autoawaytime, autoawayreason, autoawayreasonenabled, detachawayenabled, detachawayreason, detachawayreasonenabled, ident, kickreason, partreason, quitreason, sslcert, sslkey)
VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18, $19, $20)
RETURNING identityid
