
// wxEmail by Eran Ifrah (https://github.com/eranif/wxEmail)

#ifndef WXMAILER_H
#define WXMAILER_H

#include "wxEmailMessage.h"

class wxMailer
{
    wxString m_email;
    wxString m_password;
    wxString m_smtp;

public:
    /**
     * @brief construct a mailer object associated with the sender email address
     * @param email the sender's gmail email
     * @param password the email's password
     * @param smtpURL the smtp URL, for gmail it will be something like:
     * "smtps://smtp.gmail.com:465"
     */
    wxMailer(const wxString& email, const wxString& password, const wxString& smtpURL);
    virtual ~wxMailer();

    /**
     * @brief send an email
     * @param message
     */
    bool Send(const wxEmailMessage& message);
};

#endif // WXMAILER_H
