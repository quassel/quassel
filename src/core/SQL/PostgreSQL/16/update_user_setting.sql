UPDATE user_setting
SET settingvalue = $1
WHERE userid = $2 AND settingname = $3
