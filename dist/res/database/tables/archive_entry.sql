BEGIN;

CREATE TABLE archive_entry (
    archive_id INTEGER REFERENCES archive_file (id) ON DELETE CASCADE,
    id         INTEGER,
    path       TEXT,
    [index]    INTEGER,
    name       TEXT,
    size       INTEGER,
    hash       TEXT,
    type_id    TEXT,
    CONSTRAINT primary_key UNIQUE (
        archive_id,
        id
    )
);

CREATE INDEX entry_match ON archive_entry (
    name,
    path,
    hash
);

COMMIT;
