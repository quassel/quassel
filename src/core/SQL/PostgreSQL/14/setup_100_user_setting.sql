CREATE TABLE user_setting (
    userid integer NOT NULL REFERENCES quasseluser (userid) ON DELETE CASCADE,
    settingname TEXT NOT NULL,
    settingvalue bytea,
    PRIMARY KEY (userid, settingname)
)
