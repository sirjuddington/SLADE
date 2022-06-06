BEGIN;

CREATE TABLE window_info (
    window_id TEXT    PRIMARY KEY
                      NOT NULL,
    left      INTEGER,
    top       INTEGER,
    width     INTEGER,
    height    INTEGER
);

COMMIT;
