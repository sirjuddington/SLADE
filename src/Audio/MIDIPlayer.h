#pragma once

namespace sf
{
class Clock;
}

namespace slade::audio
{
class MIDIPlayer
{
public:
	MIDIPlayer();
	virtual ~MIDIPlayer();

	virtual bool isSoundfontLoaded() = 0;
	virtual bool reloadSoundfont() { return true; }

	virtual bool openFile(const string& filename) = 0;
	virtual bool openData(MemChunk& mc)           = 0;

	virtual bool isReady() = 0;

	virtual bool play()  = 0;
	virtual bool pause() = 0;
	virtual bool stop()  = 0;

	virtual bool isPlaying() = 0;
	virtual int  position()  = 0;

	virtual bool setPosition(int pos)  = 0;
	virtual bool setVolume(int volume) = 0;

protected:
	string                file_;
	MemChunk              data_;
	unique_ptr<sf::Clock> timer_;
};

class NullMIDIPlayer : public MIDIPlayer
{
public:
	bool isSoundfontLoaded() override { return false; }
	bool openFile(const string& filename) override { return false; }
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

MIDIPlayer& midiPlayer();
void        resetMIDIPlayer();
int         midiLength(const MemChunk& data);
string      midiInfo(const MemChunk& data);
} // namespace slade::audio
