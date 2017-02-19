
// wxEmail by Eran Ifrah (https://github.com/eranif/wxEmail)

#include <wx/crt.h>
#include "wxEmailMessageID.h"

wxEmailMessageID::wxEmailMessageID() {}

wxEmailMessageID::~wxEmailMessageID() {}

wxString wxEmailMessageID::Decrypt(const wxChar byte) const
{
    wxString dec = fromHexString(m_value);
    dec = XOR(dec, byte);
    return dec;
}

wxString wxEmailMessageID::Encrypt(const wxChar byte) const
{
    wxString enc = XOR(m_value, byte);
    enc = toHexString(enc);
    return enc;
}

wxEmailMessageID::wxEmailMessageID(const wxString& value)
    : m_value(value)
{
}

wxString wxEmailMessageID::toHexString(const wxString& value) const
{
    wxString output;
    for(size_t i = 0; i < value.length(); ++i) {
        wxChar buf[5] = { 0, 0, 0, 0, 0 };
        wxSprintf(buf, "%04X", (int)value[i]);
        output << buf;
    }
    return output;
}

wxString wxEmailMessageID::fromHexString(const wxString& hexString) const
{
    wxString output;
    size_t cnt = hexString.length() / 4;
    for(size_t i = 0; cnt > i; ++i) {
        wxString dec = hexString.Mid(i * 4, 4);
        int hexVal;
        wxSscanf(dec, "%X", &hexVal);
        output << (wxChar)hexVal;
    }
    return output;
}

wxString wxEmailMessageID::XOR(const wxString& str, const wxChar KEY) const
{
    wxString output;
    for(size_t i = 0; i < str.length(); ++i) {

        wxChar ch = str[i];
        wxChar x = (ch ^ KEY);
        output << x;
    }
    return output;
}
