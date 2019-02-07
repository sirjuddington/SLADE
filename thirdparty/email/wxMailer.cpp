
// wxEmail by Eran Ifrah (https://github.com/eranif/wxEmail)

#include "wxMailer.h"
#ifndef NOCURL
#include <curl/curl.h>
#endif
#ifdef __WXMSW__
#include "wx/msw/winundef.h"
#endif
#include <wx/msgdlg.h>

wxMailer::wxMailer(const wxString& email, const wxString& password, const wxString& smtpURL)
    : m_email(email)
    , m_password(password)
    , m_smtp(smtpURL)
{
}

wxMailer::~wxMailer() {}

static size_t payload_source(void* ptr, size_t size, size_t nmemb, void* userp)
{
    wxEmailMessage* message = (wxEmailMessage*)userp;
    if(!message->HasMore()) {
        return 0;
    }

    if((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
        return 0;
    }

    size_t len = (message->Len() > (size * nmemb)) ? (size * nmemb) : message->Len();
    memcpy(ptr, message->AsChar(), len);
    message->Consume(len);
    return len;
}

bool wxMailer::Send(const wxEmailMessage& message)
{
#ifndef NOCURL
    CURL* curl;
    curl = curl_easy_init();

    if(curl) {
        // Transport initialization
        curl_easy_setopt(curl, CURLOPT_USERNAME, m_email.mb_str(wxConvUTF8).data());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, m_password.mb_str(wxConvUTF8).data());
        curl_easy_setopt(curl, CURLOPT_URL, m_smtp.mb_str(wxConvUTF8).data());
        curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        // Message details
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, message.GetFrom().mb_str(wxConvUTF8).data());
        curl_slist* recipients = NULL;
        recipients = curl_slist_append(recipients, message.GetTo().mb_str(wxConvUTF8).data());
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA, (void*)&message);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        CURLcode res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            wxMessageBox(curl_easy_strerror(res), "wxEmail Error!", wxICON_ERROR | wxOK | wxCENTER);
            curl_slist_free_all(recipients);
            curl_easy_cleanup(curl);
            return false;
        }

        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
    }
    return true;
#else
    return false;
#endif
}
