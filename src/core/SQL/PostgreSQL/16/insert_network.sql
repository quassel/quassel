INSERT INTO network (networkname, identityid, servercodec, encodingcodec, decodingcodec, userandomserver, perform, useautoidentify, autoidentifyservice, autoidentifypassword, useautoreconnect, autoreconnectinterval, autoreconnectretries, unlimitedconnectretries, rejoinchannels, usesasl, saslaccount, saslpassword, userid)
VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18, $19)
RETURNING networkid
