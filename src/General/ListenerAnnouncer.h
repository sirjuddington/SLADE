
#ifndef __LISTENERANNOUNCER_H__
#define __LISTENERANNOUNCER_H__

class Announcer;

class Listener
{
private:
	vector<Announcer*>	announcers;
	bool				deaf;

public:
	Listener();
	virtual ~Listener();

	void listenTo(Announcer* a);
	void stopListening(Announcer* a);
	void clearAnnouncers() { announcers.clear(); }
	virtual void onAnnouncement(Announcer* announcer, string event_name, MemChunk& event_data);

	bool	isDeaf() { return deaf; }
	void	setDeaf(bool d) { deaf = d; }
};

class Announcer
{
private:
	vector<Listener*>	listeners;
	bool				muted;

public:
	Announcer();
	~Announcer();

	void addListener(Listener* l);
	void removeListener(Listener* l);
	void announce(string event_name, MemChunk& event_data);
	void announce(string event_name);

	bool	isMuted() { return muted; }
	void	setMuted(bool m) { muted = m; }
};

#endif //__LISTENERANNOUNCER_H__
