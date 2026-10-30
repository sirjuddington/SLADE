-- Add slade_version column to db_info table
ALTER TABLE db_info ADD COLUMN slade_version TEXT;
UPDATE db_info SET slade_version = '3.2.12';
