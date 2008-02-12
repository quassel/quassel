UPDATE buffer
SET buffertype = 2
WHERE buffercname LIKE '#%' OR buffercname LIKE '&%'
