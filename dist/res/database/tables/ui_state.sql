BEGIN;

CREATE TABLE ui_state
(
    name       TEXT,
    value      NONE,
    archive_id INTEGER REFERENCES archive_file (id) ON DELETE CASCADE,
    UNIQUE (
            name,
            archive_id
        )
);

COMMIT;
