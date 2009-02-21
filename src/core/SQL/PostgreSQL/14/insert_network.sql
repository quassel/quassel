INSERT INTO network (userid, networkname, identityid, servercodec, encodingcodec, decodingcodec, userandomserver, perform, useautoidentify, autoidentifyservice, autoidentifypassword, useautoreconnect, autoreconnectinterval, autoreconnectretries, unlimitedconnectretries, rejoinchannels)
VALUES (:userid, :networkname, :identityid, :servercodec, :encodingcodec, :decodingcodec, :userandomserver, :perform, :useautoidentify, :autoidentifyservice, :autoidentifypassword, :useautoreconnect, :autoreconnectinterval, :autoreconnectretries, :unlimitedconnectretries, :rejoinchannels)
RETURNING networkid
