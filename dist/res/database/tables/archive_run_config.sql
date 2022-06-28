BEGIN;

CREATE TABLE archive_run_config (
    archive_id    INTEGER PRIMARY KEY
                          REFERENCES archive_file (id) ON DELETE CASCADE,
    executable_id TEXT,
    run_config    INTEGER,
    run_extra     TEXT
);

COMMIT;
