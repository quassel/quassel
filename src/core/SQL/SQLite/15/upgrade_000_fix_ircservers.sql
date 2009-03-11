UPDATE ircserver
SET ssl = length(replace(replace(replace(ssl,'true','1'), 'false', '0'), '0', '')),
    useproxy = length(replace(replace(replace(useproxy,'true','1'), 'false', '0'), '0', ''))
