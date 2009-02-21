UPDATE network SET
networkname = :networkname,
identityid = :identityid,
servercodec = :servercodec,
encodingcodec = :encodingcodec,
decodingcodec = :decodingcodec,
userandomserver = :userandomserver,
perform = :perform,
useautoidentify = :useautoidentify,
autoidentifyservice = :autoidentifyservice,
autoidentifypassword = :autoidentifypassword,
useautoreconnect = :useautoreconnect,
autoreconnectinterval = :autoreconnectinterval,
autoreconnectretries = :autoreconnectretries,
unlimitedconnectretries = :unlimitedconnectretries,
rejoinchannels = :rejoinchannels
WHERE networkid = :networkid
