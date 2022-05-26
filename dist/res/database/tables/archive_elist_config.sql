BEGIN TRANSACTION;

CREATE TABLE archive_elist_config (
    archive_id      INTEGER PRIMARY KEY
                            REFERENCES archive_file (id) ON DELETE CASCADE,
    index_visible   BOOLEAN,
    index_width     INTEGER,
    name_width      INTEGER,
    size_visible    BOOLEAN,
    size_width      INTEGER,
    type_visible    BOOLEAN,
    type_width      INTEGER,
    sort_column     TEXT,
    sort_descending BOOLEAN
);

COMMIT TRANSACTION;
