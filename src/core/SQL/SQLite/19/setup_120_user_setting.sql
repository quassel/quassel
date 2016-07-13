CREATE TABLE user_setting (
    userid INTEGER NOT NULL,
    settingname TEXT NOT NULL,
    settingvalue BLOB,
    PRIMARY KEY (userid, settingname)
)
