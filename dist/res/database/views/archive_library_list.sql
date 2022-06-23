BEGIN;

CREATE VIEW archive_library_list AS
    SELECT archive_file.id,
           archive_file.path,
           archive_file.size,
           archive_file.format_id,
           archive_file.last_opened,
           archive_file.last_modified,
           COUNT(archive_entry.id) AS entry_count
      FROM archive_file
           JOIN
           archive_entry ON archive_entry.archive_id = archive_file.id
     GROUP BY archive_file.id
     ORDER BY archive_file.path;

COMMIT;
