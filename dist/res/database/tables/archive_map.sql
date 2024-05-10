BEGIN;

CREATE TABLE archive_map (
    archive_id      INTEGER REFERENCES archive_file (id) ON DELETE CASCADE,
    header_entry_id INTEGER,
    name            TEXT,
    format          INTEGER,
    UNIQUE (
        archive_id,
        header_entry_id
    )
);

COMMIT;
