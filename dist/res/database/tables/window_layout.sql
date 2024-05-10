BEGIN;

CREATE TABLE window_layout (
    window_id TEXT,
    component TEXT,
    layout    TEXT,
    UNIQUE (
        window_id,
        component
    )
);

COMMIT;
