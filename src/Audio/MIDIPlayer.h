
#ifndef __MIDIPLAYER_H__
#define __MIDIPLAYER_H__

#include "common.h"

class MIDIPlayer
{
private:
	static MIDIPlayer*		instance;

#ifndef NO_FLUIDSYNTH
	fluid_settings_t*		fs_settings;
	fluid_synth_t*			fs_synth;
	fluid_player_t*			fs_player;
	fluid_audio_driver_t*	fs_adriver;
#endif

	bool					fs_initialised;
	vector<int>				fs_soundfont_ids;

	MemChunk				data;
	wxProcess				*program;
	string					file;
	sf::Clock				timer;

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

	bool	isSoundfontLoaded() { return fs_soundfont_ids.size() > 0; }
	bool	isReady();
	void	resetPlayer();

	bool	initFluidsynth();
	bool	reloadSoundfont();
	bool	openFile(string filename);
	bool	openData(MemChunk &mc);
	bool	play();
	bool	pause();
	bool	stop();
	bool	isPlaying();
	int		getPosition();
	bool	setPosition(int pos);
	int		getLength();
	bool	setVolume(int volume);
	string	getInfo();
};

// Define for less cumbersome MIDIPlayer::getInstance()
#define theMIDIPlayer MIDIPlayer::getInstance()

#endif//__MIDIPLAYER_H__
