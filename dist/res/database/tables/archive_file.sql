BEGIN;

CREATE TABLE archive_file (
	id            INTEGER  PRIMARY KEY,
	path          TEXT     UNIQUE,		-- Path to the archive file on disk (or directory), must be unique in the table
    size          INTEGER,              -- Size of the file on disk (in bytes)
    hash          TEXT,                 -- Hash (xxHash 128-bit) of the file data on disk
    format_id     TEXT,                 -- Archive format id (eg. 'wad')
    last_opened   DATETIME,             -- Time the archive was last opened in SLADE
    last_modified DATETIME              -- Modified time of the file on disk
);

COMMIT;
