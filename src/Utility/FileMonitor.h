#pragma once

class Archive;

class FileMonitor : public wxTimer
{
public:
	FileMonitor(std::string_view filename, bool start = true);
	virtual ~FileMonitor() = default;

	wxProcess*         process() const { return process_.get(); }
	const std::string& filename() const { return filename_; }

	virtual void fileModified() {}
	virtual void processTerminated() {}

	void Notify() override;
	void onEndProcess(wxProcessEvent& e);

protected:
	std::string filename_;
	time_t      file_modified_;

private:
	std::unique_ptr<wxProcess> process_;
};

class DB2MapFileMonitor : public FileMonitor
{
public:
	DB2MapFileMonitor(std::string_view filename, Archive* archive, std::string_view map_name);
	~DB2MapFileMonitor() = default;

	void fileModified() override;
	void processTerminated() override;

private:
	Archive*    archive_ = nullptr;
	std::string map_name_;
};
