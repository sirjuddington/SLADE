BEGIN;

CREATE VIEW archive_library_list AS
	SELECT archive_file.id,
		archive_file.path,
		archive_file.size,
		archive_file.format_id,
		archive_file.last_opened,
		archive_file.last_modified,
		archive_file.parent_id,
		(
			SELECT COUNT( * ) 
				FROM archive_entry
				WHERE archive_entry.archive_id = archive_file.id
		)
		AS entry_count,
		(
			SELECT COUNT( * ) 
				FROM archive_map
				WHERE archive_map.archive_id = archive_file.id
		)
		AS map_count
	FROM archive_file
	ORDER BY archive_file.path COLLATE NOCASE ASC;

COMMIT;
