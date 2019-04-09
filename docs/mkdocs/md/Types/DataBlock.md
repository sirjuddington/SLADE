<article-head>DataBlock</article-head>

A <type>DataBlock</type> is a simple block of binary data, with various functions to help deal with reading/writing raw binary data to the block.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">size</prop> | <type>number</type> | The size of the block (in bytes)
<prop class="ro">crc</prop> | <type>number</type> | The CRC of the data

## Constructors

<fdef>function <type>DataBlock</type>.<func>new</func>()</fdef>

Creates a new, empty <type>DataBlock</type> (<prop>size</prop> will be `0`).

---

<fdef>function <type>DataBlock</type>.<func>new</func>(<arg>size</arg>)</fdef>

Creates a new <type>DataBlock</type> of <arg>size</arg> bytes, with each byte set to `0`.

<listhead>Parameters</listhead>

* <arg>size</arg> (<type>number</type>): The size (in bytes) of the block

## Functions

### AsString

<fdef>function <type>DataBlock</type>.<func>AsString</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>string</type>: The data as a string

---
### SetData

<fdef>function <type>DataBlock</type>.<func>SetData</func>(<arg>*self*</arg>, <arg>data</arg>)</fdef>

Sets the data contained within this <type>DataBlock</type> to the given string, resizing if necessary.

<listhead>Parameters</listhead>

* <arg>data</arg> (<type>string</type>): The data to import

#### Example

```lua
local data = DataBlock.new(10)
App.LogMessage('Size is ' .. data.size) -- 10

local str = 'This is a string that is 43 characters long'
data:SetData(str)
App.LogMessage('Size is ' .. data.size) -- 43
```

---
### Clear

<fdef>function <type>DataBlock</type>.<func>Clear</func>(<arg>*self*</arg>)</fdef>

Clears all data and sets <prop>size</prop> to `0`.

---
### Resize

<fdef>function <type>DataBlock</type>.<func>Resize</func>(<arg>*self*</arg>, <arg>newSize</arg>, <arg>preserveData</arg>)</fdef>

Resizes the block to <arg>newSize</arg>.

<listhead>Parameters</listhead>

* <arg>newSize</arg> (<type>number</type>): The new size of the block. If `0` the resize will fail (use <func>[Clear](#clear)</func> instead)
* <arg>preserveData</arg> (<type>boolean</type>): If `true`, existing byte values in the block will be preserved, otherwise all bytes will be set to `0`

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the resize was successful

---
### Copy

<fdef>function <type>DataBlock</type>.<func>Copy</func>(<arg>*self*</arg>, <arg>other</arg>)</fdef>

Copies data from another <type>DataBlock</type>.

<listhead>Parameters</listhead>

* <arg>other</arg> (<type>DataBlock</type>): The <type>DataBlock</type> to copy data from

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

---
### CopyTo

<fdef>function <type>DataBlock</type>.<func>CopyTo</func>(<arg>*self*</arg>, <arg>other</arg>, <arg>offset</arg>, <arg>length</arg>)</fdef>

Copies data to another <type>DataBlock</type>.

<listhead>Parameters</listhead>

* <arg>other</arg> (<type>DataBlock</type>): The <type>DataBlock</type> to copy data to
* <arg>[offset]</arg> (<type>number</type>, default `0`): Only copy bytes starting from this (`0`-based) offset
* <arg>[length]</arg> (<type>number</type>, default `0`): Copy this number of bytes. If `0`, all bytes to the end of the block are copied

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

---
### ImportFile

<fdef>function <type>DataBlock</type>.<func>ImportFile</func>(<arg>*self*</arg>, <arg>path</arg>, <arg>offset</arg>, <arg>length</arg>)</fdef>

Imports data from a file on disk at <arg>path</arg>.

<listhead>Parameters</listhead>

* <arg>path</arg> (<type>string</type>): The path to the file on disk
* <arg>[offset]</arg> (<type>number</type>, default `0`): Only import data starting from this (`0`-based) offset in the file
* <arg>[length]</arg> (<type>number</type>, default `0`): Import this number of bytes. If `0`, all bytes to the end of the file are imported

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

---
### ExportFile

<fdef>function <type>DataBlock</type>.<func>ExportFile</func>(<arg>*self*</arg>, <arg>path</arg>, <arg>offset</arg>, <arg>length</arg>)</fdef>

Exports data to a file on disk at <arg>path</arg>.

<listhead>Parameters</listhead>

* <arg>path</arg> (<type>string</type>): The path to the file on disk. The file will be created if it doesn't already exist
* <arg>[offset]</arg> (<type>number</type>, default `0`): Only export data starting from this (`0`-based) offset
* <arg>[length]</arg> (<type>number</type>, default `0`): Export this number of bytes. If `0`, all bytes to the end of the block are exported

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

---
### FillData

<fdef>function <type>DataBlock</type>.<func>FillData</func>(<arg>*self*</arg>, <arg>value</arg>)</fdef>

Sets all bytes in the block to <arg>value</arg>.

<listhead>Parameters</listhead>

* <arg>value</arg> (<type>number</type>): The value to set all bytes to (`0`-`255`)

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if the block is empty


## Functions - Reading

### ReadInt8

<fdef>function <type>DataBlock</type>.<func>ReadInt8</func>(<arg>*self*</arg>, <arg>offset</arg>)</fdef>

Reads the byte at <arg>offset</arg> bytes into the block as an 8-bit (1 byte) *signed* integer.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read

<listhead>Returns</listhead>

* <type>number</type>: The value read, or `nil` if the <arg>offset</arg> was invalid

---
### ReadUInt8

<fdef>function <type>DataBlock</type>.<func>ReadUInt8</func>(<arg>*self*</arg>, <arg>offset</arg>)</fdef>

Reads the byte at <arg>offset</arg> bytes into the block as an 8-bit (1 byte) *unsigned* integer.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read

<listhead>Returns</listhead>

* <type>number</type>: The value read, or `nil` if the <arg>offset</arg> was invalid

---
### ReadInt16

<fdef>function <type>DataBlock</type>.<func>ReadInt16</func>(<arg>*self*</arg>, <arg>offset</arg>)</fdef>

Reads the bytes beginning at <arg>offset</arg> bytes into the block as a 16-bit (2 byte) *signed* integer.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read

<listhead>Returns</listhead>

* <type>number</type>: The value read, or `nil` if the <arg>offset</arg> was invalid

---
### ReadUInt16

<fdef>function <type>DataBlock</type>.<func>ReadUInt16</func>(<arg>*self*</arg>, <arg>offset</arg>)</fdef>

Reads the bytes beginning at <arg>offset</arg> bytes into the block as a 16-bit (2 byte) *unsigned* integer.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read

<listhead>Returns</listhead>

* <type>number</type>: The value read, or `nil` if the <arg>offset</arg> was invalid

---
### ReadInt32

<fdef>function <type>DataBlock</type>.<func>ReadInt32</func>(<arg>*self*</arg>, <arg>offset</arg>)</fdef>

Reads the bytes beginning at <arg>offset</arg> bytes into the block as a 32-bit (4 byte) *signed* integer.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read

<listhead>Returns</listhead>

* <type>number</type>: The value read, or `nil` if the <arg>offset</arg> was invalid

---
### ReadUInt32

<fdef>function <type>DataBlock</type>.<func>ReadUInt32</func>(<arg>*self*</arg>, <arg>offset</arg>)</fdef>

Reads the bytes beginning at <arg>offset</arg> bytes into the block as a 32-bit (4 byte) *unsigned* integer.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read

<listhead>Returns</listhead>

* <type>number</type>: The value read, or `nil` if the <arg>offset</arg> was invalid

---
### ReadInt64

<fdef>function <type>DataBlock</type>.<func>ReadInt64</func>(<arg>*self*</arg>, <arg>offset</arg>)</fdef>

Reads the bytes beginning at <arg>offset</arg> bytes into the block as a 64-bit (8 byte) *signed* integer.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read

<listhead>Returns</listhead>

* <type>number</type>: The value read, or `nil` if the <arg>offset</arg> was invalid

---
### ReadUInt64

<fdef>function <type>DataBlock</type>.<func>ReadUInt64</func>(<arg>*self*</arg>, <arg>offset</arg>)</fdef>

Reads the bytes beginning at <arg>offset</arg> bytes into the block as a 64-bit (8 byte) *unsigned* integer.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read

<listhead>Returns</listhead>

* <type>number</type>: The value read, or `nil` if the <arg>offset</arg> was invalid

---
### ReadString

<fdef>function <type>DataBlock</type>.<func>ReadString</func>(<arg>*self*</arg>, <arg>offset</arg>, <arg>length</arg>, <arg>nullTerminated</arg>)</fdef>

Reads <arg>length</arg> bytes beginning at <arg>offset</arg> bytes into the block as a <type>string</type>.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read
* <arg>length</arg> (<type>number</type>): The number of bytes to read
* <arg>[nullTerminated]</arg> (<type>boolean</type>, default `false`): If `true`, the string will end at the first `0` after <arg>offset</arg>, or <arg>length</arg> bytes after offset, whichever comes first

<listhead>Returns</listhead>

* <type>string</type>: The string that was read, or an empty string if the <arg>offset</arg> was invalid

**Example**

```lua
local data = DataBlock.new()
data:SetData('one two\0three')

local one = data:ReadString(0, 3)
local three = data:ReadString(8, 5)

App.LogMessage(one .. ' (' .. #one .. ')')     -- one (3)
App.LogMessage(three .. ' (' .. #three .. ')') -- three (5)

local twoThree = data:ReadString(4, 9, false)
local twoThreeNT = data:ReadString(4, 9, true)

App.LogMessage('twoThree: ' .. #twoThree)     -- twoThree: 9
App.LogMessage('twoThreeNT: ' .. #twoThreeNT) -- twoThreeNT: 3
```


## Functions - Writing

!!! attention
    Note that these functions **overwrite** data, they do not insert

### WriteInt8

<fdef>function <type>DataBlock</type>.<func>WriteInt8</func>(<arg>*self*</arg>, <arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>)</fdef>

Writes <arg>value</arg> as an 8-bit (1 byte) *signed* integer at <arg>offset</arg> bytes into the block.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>number</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the value was written successfully

---
### WriteUInt8

<fdef>function <type>DataBlock</type>.<func>WriteUInt8</func>(<arg>*self*</arg>, <arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>)</fdef>

Writes <arg>value</arg> as an 8-bit (1 byte) *unsigned* integer at <arg>offset</arg> bytes into the block.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>number</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the value was written successfully

---
### WriteInt16

<fdef>function <type>DataBlock</type>.<func>WriteInt16</func>(<arg>*self*</arg>, <arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>)</fdef>

Writes <arg>value</arg> as a 16-bit (2 byte) *signed* integer at <arg>offset</arg> bytes into the block.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>number</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the value was written successfully

---
### WriteUInt16

<fdef>function <type>DataBlock</type>.<func>WriteUInt16</func>(<arg>*self*</arg>, <arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>)</fdef>

Writes <arg>value</arg> as a 16-bit (2 byte) *unsigned* integer at <arg>offset</arg> bytes into the block.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>number</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the value was written successfully

---
### WriteInt32

<fdef>function <type>DataBlock</type>.<func>WriteInt32</func>(<arg>*self*</arg>, <arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>)</fdef>

Writes <arg>value</arg> as a 32-bit (4 byte) *signed* integer at <arg>offset</arg> bytes into the block.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>number</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the value was written successfully

---
### WriteUInt32

<fdef>function <type>DataBlock</type>.<func>WriteUInt32</func>(<arg>*self*</arg>, <arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>)</fdef>

Writes <arg>value</arg> as a 32-bit (4 byte) *unsigned* integer at <arg>offset</arg> bytes into the block.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>number</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the value was written successfully

---
### WriteInt64

<fdef>function <type>DataBlock</type>.<func>WriteInt64</func>(<arg>*self*</arg>, <arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>)</fdef>

Writes <arg>value</arg> as a 64-bit (8 byte) *signed* integer at <arg>offset</arg> bytes into the block.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>number</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the value was written successfully

---
### WriteUInt64

<fdef>function <type>DataBlock</type>.<func>WriteUInt64</func>(<arg>*self*</arg>, <arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>)</fdef>

Writes <arg>value</arg> as a 64-bit (8 byte) *unsigned* integer at <arg>offset</arg> bytes into the block.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>number</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the value was written successfully

---
### WriteString

<fdef>function <type>DataBlock</type>.<func>WriteString</func>(<arg>*self*</arg>, <arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>)</fdef>

Writes the given string <arg>value</arg> at <arg>offset</arg> bytes into the block.

<listhead>Parameters</listhead>

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>string</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the value was written successfully
