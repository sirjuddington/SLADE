BEGIN;

CREATE TABLE session (
    id               INTEGER  PRIMARY KEY,
    opened_time      DATETIME,
    closed_time      DATETIME,
    version_major    INTEGER,
    version_minor    INTEGER,
    version_revision INTEGER,
    version_beta     INTEGER
);

COMMIT;
