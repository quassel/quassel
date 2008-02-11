SELECT networkid, networkname, identityid, usecustomencoding, encodingcodec, decodingcodec,
       userandomserver, perform, useautoidentify, autoidentifyservice, autoidentifypassword,
       useautoreconnect, autoreconnectinterval, autoreconnectretries, rejoinchannels
FROM network
WHERE userid = :userid