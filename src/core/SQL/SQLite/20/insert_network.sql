INSERT INTO network (userid, networkname, identityid, servercodec, encodingcodec, decodingcodec,
                     userandomserver, perform, useautoidentify, autoidentifyservice,
                     autoidentifypassword, useautoreconnect, autoreconnectinterval,
                     autoreconnectretries, unlimitedconnectretries, rejoinchannels, usesasl,
                     saslaccount, saslpassword, usecustomessagerate, messagerateburstsize,
                     messageratedelay, unlimitedmessagerate)
VALUES (:userid, :networkname, :identityid, :servercodec, :encodingcodec, :decodingcodec,
        :userandomserver, :perform, :useautoidentify, :autoidentifyservice, :autoidentifypassword,
        :useautoreconnect, :autoreconnectinterval, :autoreconnectretries, :unlimitedconnectretries,
        :rejoinchannels, :usesasl, :saslaccount, :saslpassword, :usecustomessagerate,
        :messagerateburstsize, :messageratedelay, :unlimitedmessagerate)
