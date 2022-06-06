BEGIN;

CREATE TABLE archive_bookmark (
    archive_id INTEGER REFERENCES archive_file (id) ON DELETE CASCADE,
    entry_id   INTEGER,
    UNIQUE (
        archive_id,
        entry_id
    )
);

COMMIT;
