UPDATE user_setting
SET settingvalue = :settingvalue
WHERE userid = :userid AND settingname = :settingname
