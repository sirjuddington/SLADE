BEGIN;

CREATE TABLE archive_ui_config (
    archive_id            INTEGER PRIMARY KEY
                                  REFERENCES archive_file (id) ON DELETE CASCADE,
    elist_index_visible   BOOLEAN,
    elist_index_width     INTEGER,
    elist_name_width      INTEGER,
    elist_size_visible    BOOLEAN,
    elist_size_width      INTEGER,
    elist_type_visible    BOOLEAN,
    elist_type_width      INTEGER,
    elist_sort_column     TEXT,
    elist_sort_descending BOOLEAN,
    splitter_position     INTEGER
);

COMMIT;
