#pragma once

class Announcer;

class Listener
{
public:
	Listener() = default;
	virtual ~Listener();

	void         listenTo(Announcer* a);
	void         stopListening(Announcer* a);
	void         clearAnnouncers() { announcers_.clear(); }
	virtual void onAnnouncement(Announcer* announcer, const wxString& event_name, MemChunk& event_data);

	bool isDeaf() const { return deaf_; }
	void setDeaf(bool d) { deaf_ = d; }

private:
	vector<Announcer*> announcers_;
	bool               deaf_ = false;
};

class Announcer
{
public:
	Announcer() = default;
	virtual ~Announcer();

	void addListener(Listener* l);
	void removeListener(Listener* l);
	void announce(const wxString& event_name, MemChunk& event_data);
	void announce(const wxString& event_name);

	bool isMuted() const { return muted_; }
	void setMuted(bool m) { muted_ = m; }

private:
	vector<Listener*> listeners_;
	bool              muted_ = false;
};
