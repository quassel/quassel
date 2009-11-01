SELECT identityid, identityname, realname, awaynick, awaynickenabled,
       awayreason, awayreasonenabled, autoawayenabled, autoawaytime, autoawayreason, autoawayreasonenabled,
       detachawayenabled, detachawayreason, detachawayreasonenabled, ident, kickreason, partreason, quitreason,
       sslcert, sslkey
FROM identity
WHERE userid = :userid
