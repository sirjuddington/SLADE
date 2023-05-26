BEGIN;

CREATE TABLE archive_map_config (
    archive_id INTEGER REFERENCES archive_file (id) ON DELETE CASCADE,
    game       TEXT,
    port       TEXT
);

COMMIT;
