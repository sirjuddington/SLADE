BEGIN;

CREATE TABLE archive_entry_property (
    archive_id INTEGER REFERENCES archive_file (id) ON DELETE CASCADE,
    entry_id   INTEGER,
    [key]      TEXT,
    value_type INTEGER,
    value      NONE,
    UNIQUE (
        archive_id,
        entry_id,
        [key]
    )
);

COMMIT;
