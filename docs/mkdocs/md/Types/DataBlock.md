A <type>DataBlock</type> is a simple block of binary data, with various functions to help deal with reading/writing raw binary data to the block.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">size</prop> | <type>number</type> | The size of the block (in bytes)
<prop class="ro">crc</prop> | <type>number</type> | The CRC of the data

## Constructors

<code><type>DataBlock</type>.<func>new</func>()</code>

Creates a new, empty <type>DataBlock</type> (<prop>size</prop> will be `0`).

---

<code><type>DataBlock</type>.<func>new</func>(<arg>size</arg>)</code>

Creates a new <type>DataBlock</type> of <arg>size</arg> bytes, with each byte set to `0`.

#### Parameters

* <arg>size</arg> (<type>number</type>): The size (in bytes) of the block


## Functions

### Overview

#### General

<fdef>[AsString](#asstring)() -> <type>string</type></fdef>
<fdef>[SetData](#setdata)(<arg>data</arg>)</fdef>
<fdef>[Clear](#clear)()</fdef>
<fdef>[Resize](#resize)(<arg>newSize</arg>, <arg>preserveData</arg>) -> <type>boolean</type></fdef>
<fdef>[Copy](#copy)(<arg>other</arg>) -> <type>boolean</type></fdef>
<fdef>[CopyTo](#copyto)(<arg>other</arg>, <arg>[offset]</arg>, <arg>[length]</arg>) -> <type>boolean</type></fdef>
<fdef>[ImportFile](#importfile)(<arg>path</arg>, <arg>[offset]</arg>, <arg>[length]</arg>) -> <type>boolean</type></fdef>
<fdef>[ExportFile](#exportfile)(<arg>path</arg>, <arg>[offset]</arg>, <arg>[length]</arg>) -> <type>boolean</type></fdef>
<fdef>[FillData](#filldata)(<arg>value</arg>) -> <type>boolean</type></fdef>

#### Reading

<fdef>[ReadInt8](#readint8)(<arg>offset</arg>) -> <type>number</type></fdef>
<fdef>[ReadUInt8](#readuint8)(<arg>offset</arg>) -> <type>number</type></fdef>
<fdef>[ReadInt16](#readint16)(<arg>offset</arg>) -> <type>number</type></fdef>
<fdef>[ReadUInt16](#readuint16)(<arg>offset</arg>) -> <type>number</type></fdef>
<fdef>[ReadInt32](#readint32)(<arg>offset</arg>) -> <type>number</type></fdef>
<fdef>[ReadUInt32](#readuint32)(<arg>offset</arg>) -> <type>number</type></fdef>
<fdef>[ReadInt64](#readint64)(<arg>offset</arg>) -> <type>number</type></fdef>
<fdef>[ReadUInt64](#readuint64)(<arg>offset</arg>) -> <type>number</type></fdef>
<fdef>[ReadString](#readstring)(<arg>offset</arg>, <arg>length</arg>, <arg>[nullTerminated]</arg>) -> <type>string</type></fdef>

#### Writing

!!! attention
    Note that these functions **overwrite** data, they do not insert

<fdef>[WriteInt8](#writeint8)(<arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>) -> <type>boolean</type></fdef>
<fdef>[WriteUInt8](#writeuint8)(<arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>) -> <type>boolean</type></fdef>
<fdef>[WriteInt16](#writeint16)(<arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>) -> <type>boolean</type></fdef>
<fdef>[WriteUInt16](#writeuint16)(<arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>) -> <type>boolean</type></fdef>
<fdef>[WriteInt32](#writeint32)(<arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>) -> <type>boolean</type></fdef>
<fdef>[WriteUInt32](#writeuint32)(<arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>) -> <type>boolean</type></fdef>
<fdef>[WriteInt64](#writeint64)(<arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>) -> <type>boolean</type></fdef>
<fdef>[WriteUInt64](#writeuint64)(<arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>) -> <type>boolean</type></fdef>
<fdef>[WriteString](#writestring)(<arg>offset</arg>, <arg>value</arg>, <arg>allowExpand</arg>) -> <type>boolean</type></fdef>

---
### AsString

Gets the data as a lua <type>string</type>.

#### Returns

* <type>string</type>: The data as a string

---
### SetData

Sets the data contained within this <type>DataBlock</type> to the given string, resizing if necessary.

#### Parameters

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

Clears all data and sets <prop>size</prop> to `0`.

---
### Resize

Resizes the block to <arg>newSize</arg>.

#### Parameters

* <arg>newSize</arg> (<type>number</type>): The new size of the block. If `0` the resize will fail (use <func>[Clear](#clear)</func> instead)
* <arg>preserveData</arg> (<type>boolean</type>): If `true`, existing byte values in the block will be preserved, otherwise all bytes will be set to `0`

#### Returns

* <type>boolean</type>: `true` if the resize was successful

---
### Copy

Copies data from another <type>DataBlock</type>.

#### Parameters

* <arg>other</arg> (<type>DataBlock</type>): The <type>DataBlock</type> to copy data from

#### Returns

* <type>boolean</type>: `true` on success

---
### CopyTo

Copies data to another <type>DataBlock</type>.

#### Parameters

* <arg>other</arg> (<type>DataBlock</type>): The <type>DataBlock</type> to copy data to
* <arg>[offset]</arg> (<type>number</type>): Only copy bytes starting from this (`0`-based) offset. Default is `0`
* <arg>[length]</arg> (<type>number</type>): Copy this number of bytes. Default is `0`, which means all bytes to the end of the block will be copied

#### Returns

* <type>boolean</type>: `true` on success

---
### ImportFile

Imports data from a file on disk at <arg>path</arg>.

#### Parameters

* <arg>path</arg> (<type>string</type>): The path to the file on disk
* <arg>[offset]</arg> (<type>number</type>): Only import data starting from this (`0`-based) offset in the file. Default is `0`
* <arg>[length]</arg> (<type>number</type>): Import this number of bytes. Default is `0`, which means all bytes to the end of the file will be imported

#### Returns

* <type>boolean</type>: `true` on success

---
### ExportFile

Exports data to a file on disk at <arg>path</arg>.

#### Parameters

* <arg>path</arg> (<type>string</type>): The path to the file on disk. The file will be created if it doesn't already exist
* <arg>[offset]</arg> (<type>number</type>): Only export data starting from this (`0`-based) offset. Default is `0`
* <arg>[length]</arg> (<type>number</type>): Export this number of bytes. Default is `0`, which means all bytes to the end of the block will be exported

#### Returns

* <type>boolean</type>: `true` on success

---
### FillData

Sets all bytes in the block to <arg>value</arg>.

#### Parameters

* <arg>value</arg> (<type>number</type>): The value to set all bytes to (`0`-`255`)

#### Returns

* <type>boolean</type>: `false` if the block is empty

---
### ReadInt8

Reads the byte at <arg>offset</arg> bytes into the block as an 8-bit (1 byte) *signed* integer.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read

#### Returns

* <type>number</type>: The value read, or `nil` if the <arg>offset</arg> was invalid

---
### ReadUInt8

Reads the byte at <arg>offset</arg> bytes into the block as an 8-bit (1 byte) *unsigned* integer.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read

#### Returns

* <type>number</type>: The value read, or `nil` if the <arg>offset</arg> was invalid

---
### ReadInt16

Reads the bytes beginning at <arg>offset</arg> bytes into the block as a 16-bit (2 byte) *signed* integer.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read

#### Returns

* <type>number</type>: The value read, or `nil` if the <arg>offset</arg> was invalid

---
### ReadUInt16

Reads the bytes beginning at <arg>offset</arg> bytes into the block as a 16-bit (2 byte) *unsigned* integer.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read

#### Returns

* <type>number</type>: The value read, or `nil` if the <arg>offset</arg> was invalid

---
### ReadInt32

Reads the bytes beginning at <arg>offset</arg> bytes into the block as a 32-bit (4 byte) *signed* integer.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read

#### Returns

* <type>number</type>: The value read, or `nil` if the <arg>offset</arg> was invalid

---
### ReadUInt32

Reads the bytes beginning at <arg>offset</arg> bytes into the block as a 32-bit (4 byte) *unsigned* integer.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read

#### Returns

* <type>number</type>: The value read, or `nil` if the <arg>offset</arg> was invalid

---
### ReadInt64

Reads the bytes beginning at <arg>offset</arg> bytes into the block as a 64-bit (8 byte) *signed* integer.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read

#### Returns

* <type>number</type>: The value read, or `nil` if the <arg>offset</arg> was invalid

---
### ReadUInt64

Reads the bytes beginning at <arg>offset</arg> bytes into the block as a 64-bit (8 byte) *unsigned* integer.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read

#### Returns

* <type>number</type>: The value read, or `nil` if the <arg>offset</arg> was invalid

---
### ReadString

Reads <arg>length</arg> bytes beginning at <arg>offset</arg> bytes into the block as a <type>string</type>.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to the start of the data to read
* <arg>length</arg> (<type>number</type>): The number of bytes to read
* <arg>[nullTerminated]</arg> (<type>boolean</type>): If `true`, the string will end at the first `0` after <arg>offset</arg>, or <arg>length</arg> bytes after offset, whichever comes first. Default is `false`

#### Returns

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

---
### WriteInt8

Writes <arg>value</arg> as an 8-bit (1 byte) *signed* integer at <arg>offset</arg> bytes into the block.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>number</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

#### Returns

* <type>boolean</type>: `true` if the value was written successfully

---
### WriteUInt8

Writes <arg>value</arg> as an 8-bit (1 byte) *unsigned* integer at <arg>offset</arg> bytes into the block.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>number</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

#### Returns

* <type>boolean</type>: `true` if the value was written successfully

---
### WriteInt16

Writes <arg>value</arg> as a 16-bit (2 byte) *signed* integer at <arg>offset</arg> bytes into the block.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>number</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

#### Returns

* <type>boolean</type>: `true` if the value was written successfully

---
### WriteUInt16

Writes <arg>value</arg> as a 16-bit (2 byte) *unsigned* integer at <arg>offset</arg> bytes into the block.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>number</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

#### Returns

* <type>boolean</type>: `true` if the value was written successfully

---
### WriteInt32

Writes <arg>value</arg> as a 32-bit (4 byte) *signed* integer at <arg>offset</arg> bytes into the block.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>number</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

#### Returns

* <type>boolean</type>: `true` if the value was written successfully

---
### WriteUInt32

Writes <arg>value</arg> as a 32-bit (4 byte) *unsigned* integer at <arg>offset</arg> bytes into the block.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>number</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

#### Returns

* <type>boolean</type>: `true` if the value was written successfully

---
### WriteInt64

Writes <arg>value</arg> as a 64-bit (8 byte) *signed* integer at <arg>offset</arg> bytes into the block.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>number</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

#### Returns

* <type>boolean</type>: `true` if the value was written successfully

---
### WriteUInt64

Writes <arg>value</arg> as a 64-bit (8 byte) *unsigned* integer at <arg>offset</arg> bytes into the block.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>number</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

#### Returns

* <type>boolean</type>: `true` if the value was written successfully

---
### WriteString

Writes the given string <arg>value</arg> at <arg>offset</arg> bytes into the block.

#### Parameters

* <arg>offset</arg> (<type>number</type>): The (`0`-based) offset to write data to
* <arg>value</arg> (<type>string</type>): The value to write
* <arg>allowExpand</arg> (<type>boolean</type>): If `true`, the data will be expanded to accomodate the written value if it goes past the end of the data, otherwise nothing will be written if <arg>offset</arg> is invalid

#### Returns

* <type>boolean</type>: `true` if the value was written successfully
