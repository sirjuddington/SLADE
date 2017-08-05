The `Archives` scripting namespace contains functions for managing archives.

## Functions

### `all`

**Returns** <type>[Archive](../Types/Archive.md)\[\]</type>

Returns an array of all currently open archives.

---
### `create`

<listhead>Parameters</listhead>

* <type>string</type> <arg>format_id</arg>: The <type>[ArchiveFormat](../Types/ArchiveFormat.md)</type> id to create

**Returns** <type>[Archive](../Types/Archive.md)</type>

Creates a new archive of the format specified in <arg>format_id</arg>. Returns `nil` if the archive could not be created (generally if the <arg>format_id</arg> is invalid).

!!! Note
    Currently only `wad` and `zip` formats are supported for creation

---
### `openFile`

<listhead>Parameters</listhead>

* <type>string</type> <arg>path</arg>: The full path + name of the archive file to open

**Returns** <type>[Archive](../Types/Archive.md)</type>

Attempts to open the file at <arg>path</arg> on disk. Returns the archive if it opened successfully, or `nil` if it could not be opened.

If opening fails, the error that occurred should be available via <code>[slade.globalError](App.md#globalerror)()</code>.

---
### `close` <sup>(1)</sup>

<listhead>Parameters</listhead>

* <type>[Archive](../Types/Archive.md)</type> <arg>archive</arg>: The archive to close

**Returns** <type>boolean</type>

Closes the given <arg>archive</arg>. Returns `false` if <arg>archive</arg> is invalid or not currently open.

---
### `close` <sup>(2)</sup>

<listhead>Parameters</listhead>

* <type>number</type> <arg>index</arg>: The index of the archive to close

**Returns** <type>boolean</type>

Closes the archive at <arg>index</arg> in the list of currently open archives (see <code>[all](#all)</code>). Returns `false` if the given <arg>index</arg> is invalid.

---
### `closeAll`

Closes all currently open archives.

---
### `fileExtensionsString`

**Returns** <type>string</type>

Returns a string with the extension filter for all supported archive file types.

See <code>[slade.browseFile](App.md#browsefile)</code> and the [Open Archive](../Examples/OpenArchive.md) example for more information.

---
### `baseResource`

**Returns** <type>[Archive](../Types/Archive.md)</type>

Returns the currently loaded base resource archive.

---
### `baseResourcePaths`

**Returns** <type>string[]</type>

Returns an array of configured base resource archive file paths.

!!! note "TODO"
    Needs a better description

---
### `openBaseResource`

<listhead>Parameters</listhead>

* <type>number</type> <arg>index</arg>: The base resource path index to open

**Returns** <type>boolean</type>

Opens the base resource archive from the path at <arg>index</arg> in the list of base resource archive file paths (see <code>[baseResourcePaths](#baseresourcepaths)</code>).

!!! note "TODO"
    Needs a better description

---
### `programResource`

**Returns** <type>[Archive](../Types/Archive.md)</type>

Returns the program resource archive (either `slade.pk3` or the `res` folder if you are running a dev build).

---
### `recentFiles`

**Returns** <type>string[]</type>

Returns an array of file paths to recently opened archives.

---
### `bookmarks`

**Returns** <type>[ArchiveEntry](../Types/ArchiveEntry.md)`[`]</type>

Returns an array of all currently bookmarked entries.

---
### `addBookmark`

<listhead>Parameters</listhead>

* <type>[ArchiveEntry](../Types/ArchiveEntry.md)</type> <arg>entry</arg>: The entry to bookmark

Adds <arg>entry</arg> as a bookmark.

---
### `removeBookmark`

<listhead>Parameters</listhead>

* <type>[ArchiveEntry](../Types/ArchiveEntry.md)</type> <arg>entry</arg>: The entry to un-bookmark

**Returns** <type>boolean</type>

Removes <arg>entry</arg> from the bookmarked entries list. Returns `false` if the given <arg>entry</arg> was not currently bookmarked.
