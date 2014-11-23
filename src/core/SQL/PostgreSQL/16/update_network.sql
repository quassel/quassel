UPDATE network SET
networkname = $1,
identityid = $2,
servercodec = $3,
encodingcodec = $4,
decodingcodec = $5,
userandomserver = $6,
perform = $7,
useautoidentify = $8,
autoidentifyservice = $9,
autoidentifypassword = $10,
useautoreconnect = $11,
autoreconnectinterval = $12,
autoreconnectretries = $13,
unlimitedconnectretries = $14,
rejoinchannels = $15,
usesasl = $16,
saslaccount = $17,
saslpassword = $18
WHERE userid = $19 AND networkid = $20

