#pragma once

#define EDF_FALSE 0
#define EDF_UNLIKELY 64
#define EDF_MAYBE 128
#define EDF_PROBABLY 192
#define EDF_TRUE 255

class EntryDataFormat
{
public:
	EntryDataFormat(string id);
	virtual ~EntryDataFormat();

	const string& getId() const { return id_; }

	virtual int isThisFormat(MemChunk& mc);
	void        copyToFormat(EntryDataFormat& target);

	static void             initBuiltinFormats();
	static bool             readDataFormatDefinition(MemChunk& mc);
	static EntryDataFormat* getFormat(string id);
	static EntryDataFormat* anyFormat();
	static EntryDataFormat* textFormat();

private:
	string id_;

	// Struct to specify an inclusive range for a byte (min <= valid <= max)
	// If max == min, only 1 valid value
	struct ByteValueRange
	{
		uint8_t min;
		uint8_t max;

		ByteValueRange()
		{
			min = 0;
			max = 255;
		}
	};

	// Struct to specify valid values for a byte at pos
	struct BytePattern
	{
		unsigned               pos;
		vector<ByteValueRange> valid_values;

		bool match(uint8_t value)
		{
			for (unsigned a = 0; a < valid_values.size(); a++)
			{
				if (value < valid_values[a].min || value > valid_values[a].max)
					return false;
			}

			return true;
		}
	};

	// Detection
	unsigned            size_min_;
	vector<BytePattern> patterns_;
	// Also needed:
	// Some way to check more complex values (eg. multiply byte 0 and 1, result must be in a certain range)
};
