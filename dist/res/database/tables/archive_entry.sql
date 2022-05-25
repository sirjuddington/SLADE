BEGIN TRANSACTION;

CREATE TABLE archive_entry (
    id         INTEGER PRIMARY KEY,
    archive_id INTEGER REFERENCES archive_file (id) ON DELETE CASCADE,
    name       TEXT,
    size       INTEGER,
    hash       TEXT,
    type_id    TEXT
);

COMMIT TRANSACTION;
