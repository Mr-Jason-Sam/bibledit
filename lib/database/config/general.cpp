/*
Copyright (©) 2003-2014 Teus Benschop.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include <database/config/general.h>
#include <filter/url.h>
#include <filter/string.h>


using namespace std;


Database_Config_General::Database_Config_General ()
{
}


Database_Config_General::~Database_Config_General ()
{
}


// Functions for getting and setting values or lists of values.


string Database_Config_General::file (const char * key)
{
  return filter_url_create_root_path ("databases", "config", "general", key);
}


string Database_Config_General::getValue (const char * key, const char * default_value)
{
  string value;
  string filename = file (key);
  if (filter_url_file_exists (filename)) value = filter_url_file_get_contents (filename);
  else value = default_value;
  return value;
}


void Database_Config_General::setValue (const char * key, string value)
{
  string filename = file (key);
  filter_url_file_put_contents (filename, value);
}


bool Database_Config_General::getValue (const char * key, bool default_value)
{
  string value = getValue (key, convert_to_string (default_value).c_str());
  return convert_to_bool (value);
}


void Database_Config_General::setValue (const char * key, bool value)
{
  setValue (key, convert_to_string (value).c_str());
}


vector <string> Database_Config_General::getList (const char * key)
{
  string contents = getValue (key, "");
  return filter_string_explode (contents, '\n');
}


void Database_Config_General::setList (const char * key, vector <string> values)
{
  string value = filter_string_implode (values, "\n");
  setValue (key, value);
}


// Named configuration functions.


string Database_Config_General::getSiteMailName ()
{
  return getValue ("site-mail-name", "Bible Translation");
}
void Database_Config_General::setSiteMailName (string value)
{
  setValue ("site-mail-name", value);
}


string Database_Config_General::getSiteMailAddress ()
{
  return getValue ("site-mail-address", "");
}
void Database_Config_General::setSiteMailAddress (string value)
{
  setValue ("site-mail-address", value);
}


string Database_Config_General::getMailStorageHost ()
{
  return getValue ("mail-storage-host", "");
}
void Database_Config_General::setMailStorageHost (string value)
{
  setValue ("mail-storage-host", value);
}


string Database_Config_General::getMailStorageUsername ()
{
  return getValue ("mail-storage-username", "");
}
void Database_Config_General::setMailStorageUsername (string value)
{
  setValue ("mail-storage-username", value);
}


string Database_Config_General::getMailStoragePassword ()
{
  return getValue ("mail-storage-password", "");
}


void Database_Config_General::setMailStoragePassword (string value)
{
  setValue ("mail-storage-password", value);
}


string Database_Config_General::getMailStorageProtocol ()
{
  return getValue ("mail-storage-protocol", "");
}
void Database_Config_General::setMailStorageProtocol (string value)
{
  setValue ("mail-storage-protocol", value);
}


string Database_Config_General::getMailStoragePort ()
{
  return getValue ("mail-storage-port", "");
}
void Database_Config_General::setMailStoragePort (string value)
{
  setValue ("mail-storage-port", value);
}


string Database_Config_General::getMailSendHost ()
{
  return getValue ("mail-send-host", "");
}
void Database_Config_General::setMailSendHost (string value)
{
  setValue ("mail-send-host", value);
}


string Database_Config_General::getMailSendUsername ()
{
  return getValue ("mail-send-username", "");
}
void Database_Config_General::setMailSendUsername (string value)
{
  setValue ("mail-send-username", value);
}


string Database_Config_General::getMailSendPassword ()
{
  return getValue ("mail-send-password", "");
}
void Database_Config_General::setMailSendPassword (string value)
{
  setValue ("mail-send-password", value);
}


string Database_Config_General::getMailSendPort ()
{
  return getValue ("mail-send-port", "");
}
void Database_Config_General::setMailSendPort (string value)
{
  setValue ("mail-send-port", value);
}


string Database_Config_General::getTimerMinute ()
{
  return getValue ("timer-minute", "");
}
void Database_Config_General::setTimerMinute (string value)
{
  setValue ("timer-minute", value);
}


string Database_Config_General::getTimezone ()
{
  return getValue ("timezone", "UTC");
}
void Database_Config_General::setTimezone (string value)
{
  setValue ("timezone", value);
}


string Database_Config_General::getSiteURL ()
{
  return getValue ("site-url", "");
}
void Database_Config_General::setSiteURL (string value)
{
  setValue ("site-url", value);
}


string Database_Config_General::getSiteLanguage ()
{
  return getValue ("site-language", "");
}
void Database_Config_General::setSiteLanguage (string value)
{
  setValue ("site-language", value);
}


bool Database_Config_General::getClientMode ()
{
  return getValue ("client-mode", false);
}
void Database_Config_General::setClientMode (bool value)
{
  setValue ("client-mode", value);
}


string Database_Config_General::getServerAddress ()
{
  return getValue ("server-address", "");
}
void Database_Config_General::setServerAddress (string value)
{
  setValue ("server-address", value);
}


string Database_Config_General::getRepeatSendReceive ()
{
  return getValue ("repeat-send-receive", "0");
}
void Database_Config_General::setRepeatSendReceive (string value)
{
  setValue ("repeat-send-receive", value);
}


string Database_Config_General::getInstalledVersion ()
{
  return getValue ("installed-version", "0");
}
void Database_Config_General::setInstalledVersion (string value)
{
  setValue ("installed-version", value);
}


