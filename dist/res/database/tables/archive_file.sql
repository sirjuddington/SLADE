BEGIN TRANSACTION;

CREATE TABLE archive_file (
    path          TEXT     NOT NULL
                           PRIMARY KEY,
    size          INTEGER,
    hash          TEXT,
    format_id     TEXT,
    last_opened   DATETIME,
    last_modified DATETIME
);

COMMIT TRANSACTION;
