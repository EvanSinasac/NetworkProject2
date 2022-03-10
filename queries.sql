SELECT MAX(userID) FROM web_auth;

SELECT MAX(userID) AS largestID FROM web_auth;

SELECT * FROM web_auth;
SELECT * FROM user;

SELECT last_insert_id();

ALTER TABLE web_auth AUTO_INCREMENT = 1;
ALTER TABLE user AUTO_INCREMENT = 1;

INSERT INTO user (last_login, creation_date) VALUES (now(), now());

SELECT id FROM web_auth WHERE email = asdf;
SELECT id FROM user WHERE id = 1;
SELECT creation_date FROM user WHERE id = 1;
UPDATE user SET last_login = now() WHERE id = 1;