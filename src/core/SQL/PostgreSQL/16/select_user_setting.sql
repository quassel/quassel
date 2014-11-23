SELECT settingvalue
FROM user_setting
WHERE userid = $1 AND settingname = $2
