#pragma once

class Archive;

class FileMonitor : public wxTimer
{
public:
	FileMonitor(const string& filename, bool start = true);
	virtual ~FileMonitor() = default;

	wxProcess* process() const { return process_.get(); }
	string     filename() const { return filename_; }

	virtual void fileModified() {}
	virtual void processTerminated() {}

	void Notify() override;
	void onEndProcess(wxProcessEvent& e);

protected:
	string filename_;
	time_t file_modified_;

private:
	std::unique_ptr<wxProcess> process_;
};

class DB2MapFileMonitor : public FileMonitor
{
public:
	DB2MapFileMonitor(const string& filename, Archive* archive, const string& map_name);
	~DB2MapFileMonitor() = default;

	void fileModified() override;
	void processTerminated() override;

private:
	Archive* archive_ = nullptr;
	string   map_name_;
};
