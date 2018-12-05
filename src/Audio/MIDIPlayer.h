#pragma once

class MIDIPlayer
{
public:
	virtual ~MIDIPlayer() = default;

	virtual bool isSoundfontLoaded() = 0;
	virtual bool reloadSoundfont() { return true; }

	virtual bool openFile(string filename) = 0;
	virtual bool openData(MemChunk& mc)    = 0;

	virtual bool isReady() = 0;

	virtual bool play()  = 0;
	virtual bool pause() = 0;
	virtual bool stop()  = 0;

	virtual bool isPlaying() = 0;
	virtual int  position()  = 0;

	virtual bool setPosition(int pos)  = 0;
	virtual bool setVolume(int volume) = 0;

	virtual int    length();
	virtual string info();

protected:
	string    file_;
	MemChunk  data_;
	sf::Clock timer_;
};

class NullMIDIPlayer : public MIDIPlayer
{
public:
	bool isSoundfontLoaded() override { return false; }
	bool openFile(string filename) override { return false; }
	bool openData(MemChunk& mc) override { return false; }
	bool play() override { return false; }
	bool pause() override { return false; }
	bool stop() override { return false; }
	bool isPlaying() override { return false; }
	int  position() override { return 0; }
	bool setPosition(int pos) override { return false; }
	bool setVolume(int volume) override { return false; }

protected:
	bool isReady() override { return false; }
};

namespace MIDI
{
MIDIPlayer& player();
void        resetPlayer();
} // namespace MIDI
