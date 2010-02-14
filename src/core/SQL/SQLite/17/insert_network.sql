INSERT INTO network (userid, networkname, identityid, servercodec, encodingcodec, decodingcodec, userandomserver,
                     perform, useautoidentify, autoidentifyservice, autoidentifypassword, useautoreconnect, autoreconnectinterval, autoreconnectretries, unlimitedconnectretries, rejoinchannels, usesasl, saslaccount, saslpassword)
VALUES (:userid, :networkname, :identityid, :servercodec, :encodingcodec, :decodingcodec, :userandomserver,
        :perform, :useautoidentify, :autoidentifyservice, :autoidentifypassword, :useautoreconnect, :autoreconnectinterval, :autoreconnectretries, :unlimitedconnectretries, :rejoinchannels, :usesasl, :saslaccount, :saslpassword)
