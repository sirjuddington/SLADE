The `SplashWindow` scripting namespace contains functions for controlling the SLADE splash window.

## Functions

### `Show`

<listhead>Parameters</listhead>

* <type>string</type> <arg>message</arg>: The message to show
* `[`<type>boolean</type> <arg>progress</arg>: `false]`: Whether to show the progress bar

Shows the splash window with <arg>message</arg>. If <arg>progress</arg> is `true`, a progress bar will be shown on the splash window (see <code>[Progress](#progress)</code>, <code>[SetProgressMessage](#setprogressmessage)</code>, <code>[SetProgress](#setprogress)</code> below).

---
### `Hide`

Hides the splash window if it is currently showing.

---
### `Update`

Updates and redraws the splash window.

---
### `Progress`

**Returns** <type>number</type>

Returns the current progress bar progress. This is a floating point number between `0.0` (empty) and `1.0` (full).

---
### `SetMessage`

<listhead>Parameters</listhead>

* <type>string</type> <arg>message</arg>: The message to show

Sets the splash window message to <arg>message</arg>.

---
### `SetProgressMessage`

<listhead>Parameters</listhead>

* <type>string</type> <arg>message</arg>: The progress bar message to show

Sets the small message within the progress bar to <arg>message</arg>.

---
### `SetProgress`

<listhead>Parameters</listhead>

* <type>number</type> <arg>progress</arg>: The progress amount

Sets the progress bar progress amount. This is a floating point number between `0.0` (empty) and `1.0` (full).
