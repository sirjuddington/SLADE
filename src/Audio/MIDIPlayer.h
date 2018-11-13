#pragma once

class MIDIPlayer
{
public:
	MIDIPlayer();
	~MIDIPlayer();

	// Singleton implementation
	static MIDIPlayer* getInstance()
	{
		if (!instance_)
			instance_ = new MIDIPlayer();

		return instance_;
	}

	bool isSoundfontLoaded() { return fs_soundfont_ids_.size() > 0; }
	bool isReady();
	void resetPlayer();

	bool   initFluidsynth();
	bool   reloadSoundfont();
	bool   openFile(string filename);
	bool   openData(MemChunk& mc);
	bool   play();
	bool   pause();
	bool   stop();
	bool   isPlaying();
	int    getPosition();
	bool   setPosition(int pos);
	int    getLength();
	bool   setVolume(int volume);
	string getInfo();

private:
	static MIDIPlayer* instance_;

#ifndef NO_FLUIDSYNTH
	fluid_settings_t*     fs_settings_;
	fluid_synth_t*        fs_synth_;
	fluid_player_t*       fs_player_;
	fluid_audio_driver_t* fs_adriver_;
#endif

	bool        fs_initialised_;
	vector<int> fs_soundfont_ids_;

	MemChunk   data_;
	wxProcess* program_;
	string     file_;
	sf::Clock  timer_;
};

// Define for less cumbersome MIDIPlayer::getInstance()
#define theMIDIPlayer MIDIPlayer::getInstance()
