ALTER TABLE IDENTITY_USER ADD COLUMN USER_UUID blob;
CREATE INDEX IDX_USER_UUID ON IDENTITY_USER (USER_UUID);

INSERT INTO SCHEMA_CHANGES(VERSION_NUMBER, SCRIPT_NAME, TIMESTAMP_UTC_APPLIED) VALUES(2, 'updatesqlitepatients2.sql', datetime());
