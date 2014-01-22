
#ifndef __MIDIPLAYER_H__
#define __MIDIPLAYER_H__

#include <fluidsynth.h>

class MIDIPlayer
{
private:
	static MIDIPlayer*	instance;

	fluid_settings_t*		fs_settings;
	fluid_synth_t*			fs_synth;
	fluid_player_t*			fs_player;
	fluid_audio_driver_t*	fs_adriver;

	bool		fs_initialised;
	vector<int>	fs_soundfont_ids;

public:
	MIDIPlayer();
	~MIDIPlayer();

	// Singleton implementation
	static MIDIPlayer*	getInstance()
	{
		if (!instance)
			instance = new MIDIPlayer();

		return instance;
	}

	bool	isInitialised() { return fs_initialised; }
	bool	isSoundfontLoaded() { return fs_soundfont_ids.size() > 0; }

	void resetPlayer()
	{
		if (instance)
		{
			delete instance;
			instance = new MIDIPlayer();
		}
	}

	bool	initFluidsynth();
	bool	reloadSoundfont();
	bool	openFile(string filename);
	bool	play();
	bool	pause();
	bool	stop();
	bool	isPlaying();
	int		getPosition();
	bool	setPosition(int pos);
	int		getLength();
	bool	setVolume(int volume);
};

// Define for less cumbersome MIDIPlayer::getInstance()
#define theMIDIPlayer MIDIPlayer::getInstance()

#endif//__MIDIPLAYER_H__
