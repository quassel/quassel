SELECT settingvalue
FROM user_setting
WHERE userid = :userid AND settingname = :settingname
