#include "Main.h"

#include "mus2midi.h"
#include "tarray.h"
#include "i_music.h"

enum EMidiDevice
{
	MDEV_DEFAULT = -1,
	MDEV_MMAPI = 0,
	MDEV_OPL = 1,
	MDEV_FMOD = 2,
	MDEV_TIMIDITY = 3,
	MDEV_FLUIDSYNTH = 4,
	MDEV_GUS = 5,
};

// Base class for streaming MUS and MIDI files ------------------------------

class MIDIStreamer
{
public:
	MIDIStreamer(EMidiDevice type);
	~MIDIStreamer();

	bool SetSubsong(int subsong);
	void CreateSMF(TArray<BYTE> &file, int looplimit=0);

protected:
	int VolumeControllerChange(int channel, int volume);
	int ClampLoopCount(int loopcount);
	void SetTempo(int new_tempo);

	// Virtuals for subclasses to override
	virtual void CheckCaps(int tech);
	virtual void DoRestart() = 0;
	virtual bool CheckDone() = 0;
	virtual bool SetMIDISubsong(int subsong);
	virtual DWORD *MakeEvents(DWORD *events, DWORD *max_event_p, DWORD max_time) = 0;

	enum
	{
		MAX_EVENTS = 128
	};

	enum
	{
		SONG_MORE,
		SONG_DONE,
		SONG_ERROR
	};

	DWORD Events[2][MAX_EVENTS*3];
	MIDIHDR Buffer[2];
	int BufferNum;
	int EndQueued;
	bool VolumeChanged;
	bool Restarting;
	bool InitialPlayback;
	DWORD NewVolume;
	int Division;
	int Tempo;
	int InitialTempo;
	BYTE ChannelVolumes[16];
	DWORD Volume;
	EMidiDevice DeviceType;
	bool CallbackIsThreaded;
	int LoopLimit;
};

// MUS file played with a MIDI stream ---------------------------------------

class MUSSong : public MIDIStreamer
{
public:
	MUSSong(FILE *file, const BYTE *musiccache, int length, EMidiDevice type);
	~MUSSong();

protected:
	void DoRestart();
	bool CheckDone();
	DWORD *MakeEvents(DWORD *events, DWORD *max_events_p, DWORD max_time);

	MUSHeader *MusHeader;
	BYTE *MusBuffer;
	BYTE LastVelocity[16];
	size_t MusP, MaxMusP;
};

// MIDI file played with a MIDI stream --------------------------------------

class MIDISong : public MIDIStreamer
{
public:
	MIDISong(FILE *file, const BYTE *musiccache, int length, EMidiDevice type);
	~MIDISong();

protected:
	void CheckCaps(int tech);
	void DoRestart();
	bool CheckDone();
	DWORD *MakeEvents(DWORD *events, DWORD *max_events_p, DWORD max_time);
	void AdvanceTracks(DWORD time);

	struct TrackInfo;

	void ProcessInitialMetaEvents ();
	DWORD *SendCommand (DWORD *event, TrackInfo *track, DWORD delay);
	TrackInfo *FindNextDue ();

	BYTE *MusHeader;
	int SongLen;
	TrackInfo *Tracks;
	TrackInfo *TrackDue;
	int NumTracks;
	int Format;
	WORD DesignationMask;
};

// HMI file played with a MIDI stream ---------------------------------------

struct AutoNoteOff
{
	DWORD Delay;
	BYTE Channel, Key;
};
// Sorry, std::priority_queue, but I want to be able to modify the contents of the heap.
class NoteOffQueue : public TArray<AutoNoteOff>
{
public:
	void AddNoteOff(DWORD delay, BYTE channel, BYTE key);
	void AdvanceTime(DWORD time);
	bool Pop(AutoNoteOff &item);

protected:
	void Heapify();

	unsigned int Parent(unsigned int i) { return (i + 1u) / 2u - 1u; }
	unsigned int Left(unsigned int i) { return (i + 1u) * 2u - 1u; }
	unsigned int Right(unsigned int i) { return (i + 1u) * 2u; }
};

class HMISong : public MIDIStreamer
{
public:
	HMISong(FILE *file, const BYTE *musiccache, int length, EMidiDevice type);
	~HMISong();

protected:
	void SetupForHMI(int len);
	void SetupForHMP(int len);
	void CheckCaps(int tech);

	void DoRestart();
	bool CheckDone();
	DWORD *MakeEvents(DWORD *events, DWORD *max_events_p, DWORD max_time);
	void AdvanceTracks(DWORD time);

	struct TrackInfo;

	void ProcessInitialMetaEvents ();
	DWORD *SendCommand (DWORD *event, TrackInfo *track, DWORD delay);
	TrackInfo *FindNextDue ();

	static DWORD ReadVarLenHMI(TrackInfo *);
	static DWORD ReadVarLenHMP(TrackInfo *);

	BYTE *MusHeader;
	int SongLen;
	int NumTracks;
	TrackInfo *Tracks;
	TrackInfo *TrackDue;
	TrackInfo *FakeTrack;
	DWORD (*ReadVarLen)(TrackInfo *);
	NoteOffQueue NoteOffs;
};

// XMI file played with a MIDI stream ---------------------------------------

class XMISong : public MIDIStreamer
{
public:
	XMISong(FILE *file, const BYTE *musiccache, int length, EMidiDevice type);
	~XMISong();

protected:
	struct TrackInfo;
	enum EventSource { EVENT_None, EVENT_Real, EVENT_Fake };

	int FindXMIDforms(const BYTE *chunk, int len, TrackInfo *songs) const;
	void FoundXMID(const BYTE *chunk, int len, TrackInfo *song) const;
	bool SetMIDISubsong(int subsong);
	void DoRestart();
	bool CheckDone();
	DWORD *MakeEvents(DWORD *events, DWORD *max_events_p, DWORD max_time);
	void AdvanceSong(DWORD time);

	void ProcessInitialMetaEvents();
	DWORD *SendCommand (DWORD *event, EventSource track, DWORD delay);
	EventSource FindNextDue();

	BYTE *MusHeader;
	int SongLen;		// length of the entire file
	int NumSongs;
	TrackInfo *Songs;
	TrackInfo *CurrSong;
	NoteOffQueue NoteOffs;
	EventSource EventDue;
};
