
#ifndef __VERSION_CHECK_H__
#define __VERSION_CHECK_H__

#include "common.h"

wxDECLARE_EVENT(wxEVT_COMMAND_VERSIONCHECK_COMPLETED, wxThreadEvent);

class VersionCheck : public wxThread
{
private:
	wxEvtHandler*	handler;

public:
	VersionCheck(wxEvtHandler* handler);
	virtual ~VersionCheck();

	ExitCode Entry();
};

#endif//__VERSION_CHECK_H__
