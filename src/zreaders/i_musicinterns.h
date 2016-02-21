#include "Main.h"

#include "mus2midi.h"
#include "tarray.h"
#include "i_music.h"

// Base class for streaming MUS and MIDI files ------------------------------

class MIDIStreamer
{
public:
	MIDIStreamer();
	virtual ~MIDIStreamer();

	int  GetSubsongs();
	bool SetSubsong(int subsong);
	void CreateSMF(TArray<uint8_t> &file, int looplimit=0);

protected:
	int VolumeControllerChange(int channel, int volume);
	int ClampLoopCount(int loopcount);
	void SetTempo(int new_tempo);

	// Virtuals for subclasses to override
	virtual void CheckCaps(int tech);
	virtual void DoRestart() = 0;
	virtual bool CheckDone() = 0;
	virtual int  GetMIDISubsongs();
	virtual bool SetMIDISubsong(int subsong);
	virtual uint32_t *MakeEvents(uint32_t *events, uint32_t *max_event_p, uint32_t max_time) = 0;

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

	uint32_t Events[2][MAX_EVENTS*3];
	int Division;
	int Tempo;
	int InitialTempo;
	uint32_t Volume;
	int LoopLimit;
};

// MUS file played with a MIDI stream ---------------------------------------

class MUSSong : public MIDIStreamer
{
public:
	MUSSong(FILE *file, const uint8_t *musiccache, int length);
	~MUSSong();

protected:
	void DoRestart();
	bool CheckDone();
	uint32_t *MakeEvents(uint32_t *events, uint32_t *max_events_p, uint32_t max_time);

	MUSHeader *MusHeader;
	uint8_t *MusBuffer;
	uint8_t LastVelocity[16];
	size_t MusP, MaxMusP;
};

// MIDI file played with a MIDI stream --------------------------------------

class MIDISong : public MIDIStreamer
{
public:
	MIDISong(FILE *file, const uint8_t *musiccache, int length);
	~MIDISong();

protected:
	void CheckCaps(int tech);
	void DoRestart();
	bool CheckDone();
	uint32_t *MakeEvents(uint32_t *events, uint32_t *max_events_p, uint32_t max_time);
	void AdvanceTracks(uint32_t time);

	struct TrackInfo;

	void ProcessInitialMetaEvents ();
	uint32_t *SendCommand (uint32_t *event, TrackInfo *track, uint32_t delay);
	TrackInfo *FindNextDue ();

	uint8_t *MusHeader;
	int SongLen;
	TrackInfo *Tracks;
	TrackInfo *TrackDue;
	int NumTracks;
	int Format;
	uint16_t DesignationMask;
};

// HMI file played with a MIDI stream ---------------------------------------

struct AutoNoteOff
{
	uint32_t Delay;
	uint8_t Channel, Key;
};
// Sorry, std::priority_queue, but I want to be able to modify the contents of the heap.
class NoteOffQueue : public TArray<AutoNoteOff>
{
public:
	void AddNoteOff(uint32_t delay, uint8_t channel, uint8_t key);
	void AdvanceTime(uint32_t time);
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
	HMISong(FILE *file, const uint8_t *musiccache, int length);
	~HMISong();

protected:
	void SetupForHMI(int len);
	void SetupForHMP(int len);
	void CheckCaps(int tech);

	void DoRestart();
	bool CheckDone();
	uint32_t *MakeEvents(uint32_t *events, uint32_t *max_events_p, uint32_t max_time);
	void AdvanceTracks(uint32_t time);

	struct TrackInfo;

	void ProcessInitialMetaEvents ();
	uint32_t *SendCommand (uint32_t *event, TrackInfo *track, uint32_t delay);
	TrackInfo *FindNextDue ();

	static uint32_t ReadVarLenHMI(TrackInfo *);
	static uint32_t ReadVarLenHMP(TrackInfo *);

	uint8_t *MusHeader;
	int SongLen;
	int NumTracks;
	TrackInfo *Tracks;
	TrackInfo *TrackDue;
	TrackInfo *FakeTrack;
	uint32_t (*ReadVarLen)(TrackInfo *);
	NoteOffQueue NoteOffs;
};

// XMI file played with a MIDI stream ---------------------------------------

class XMISong : public MIDIStreamer
{
public:
	XMISong(FILE *file, const uint8_t *musiccache, int length);
	~XMISong();

protected:
	struct TrackInfo;
	enum EventSource { EVENT_None, EVENT_Real, EVENT_Fake };

	int FindXMIDforms(const uint8_t *chunk, int len, TrackInfo *songs) const;
	void FoundXMID(const uint8_t *chunk, int len, TrackInfo *song) const;
	int  GetMIDISubsongs();
	bool SetMIDISubsong(int subsong);
	void DoRestart();
	bool CheckDone();
	uint32_t *MakeEvents(uint32_t *events, uint32_t *max_events_p, uint32_t max_time);
	void AdvanceSong(uint32_t time);

	void ProcessInitialMetaEvents();
	uint32_t *SendCommand (uint32_t *event, EventSource track, uint32_t delay);
	EventSource FindNextDue();

	uint8_t *MusHeader;
	int SongLen;		// length of the entire file
	int NumSongs;
	TrackInfo *Songs;
	TrackInfo *CurrSong;
	NoteOffQueue NoteOffs;
	EventSource EventDue;
};


// Stuff that doesn't exist on Linux apparently

#ifndef MOD_MIDIPORT
/* flags for wTechnology field of MIDIOUTCAPS structure */
#define MOD_MIDIPORT    1  /* output port */
#define MOD_SYNTH       2  /* generic internal synth */
#define MOD_SQSYNTH     3  /* square wave internal synth */
#define MOD_FMSYNTH     4  /* FM internal synth */
#define MOD_MAPPER      5  /* MIDI mapper */
#define MOD_WAVETABLE   6  /* hardware wavetable synth */
#define MOD_SWSYNTH     7  /* software synth */
#endif

#ifndef MEVT_F_SHORT
#define MEVT_F_SHORT        0x00000000L
#define MEVT_F_LONG         0x80000000L
#define MEVT_F_CALLBACK     0x40000000L

#define MEVT_EVENTTYPE(x)   ((uint8_t)(((x)>>24)&0xFF))
#define MEVT_EVENTPARM(x)   ((uint32_t)((x)&0x00FFFFFFL))

#define MEVT_SHORTMSG       ((uint8_t)0x00)    /* parm = shortmsg for midiOutShortMsg */
#define MEVT_TEMPO          ((uint8_t)0x01)    /* parm = new tempo in microsec/qn     */
#define MEVT_NOP            ((uint8_t)0x02)    /* parm = unused; does nothing         */

/* 0x04-0x7F reserved */

#define MEVT_LONGMSG        ((uint8_t)0x80)    /* parm = bytes to send verbatim       */
#define MEVT_COMMENT        ((uint8_t)0x82)    /* parm = comment data                 */
#define MEVT_VERSION        ((uint8_t)0x84)    /* parm = MIDISTRMBUFFVER struct       */
#endif