#pragma once

class EntryDataFormat
{
public:
	// Standard values for isThisFormat return value
	static const int MATCH_FALSE    = 0;
	static const int MATCH_UNLIKELY = 64;
	static const int MATCH_MAYBE    = 128;
	static const int MATCH_PROBABLY = 192;
	static const int MATCH_TRUE     = 255;

	EntryDataFormat(string_view id) : id_{ id } {}
	virtual ~EntryDataFormat() = default;

	const string& id() const { return id_; }

	virtual int isThisFormat(MemChunk& mc);
	void        copyToFormat(EntryDataFormat& target) const;

	static void             initBuiltinFormats();
	static bool             readDataFormatDefinition(MemChunk& mc);
	static EntryDataFormat* format(string_view id);
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
	unsigned            size_min_ = 0;
	vector<BytePattern> patterns_;
	// Also needed:
	// Some way to check more complex values (eg. multiply byte 0 and 1, result must be in a certain range)
};
