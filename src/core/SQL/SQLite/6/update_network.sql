UPDATE network SET
networkname = :networkname,
identityid = :identityid,
usecustomencoding = :usecustomencoding,
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
rejoinchannels = :rejoinchannels
WHERE networkid = :networkid
