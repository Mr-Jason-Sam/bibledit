/*
Copyright (©) 2003-2015 Teus Benschop.

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


#include <config/logic.h>
#include <filter/string.h>
#include <filter/url.h>
#include <database/books.h>
#include <database/config/general.h>
#include <config.h>
#include <cstdlib>


// Returns the Bibledit version number.
const char * config_logic_version ()
{
  return VERSION;
}


// Return the network port configured for the server.
const char * config_logic_network_port ()
{
  return NETWORK_PORT;
}


// Returns whether client mode is enabled during configure.
bool config_logic_client_prepared ()
{
#ifdef CLIENT_PREPARED
  return true;
#endif
  return false;
}


// Returns whether demo mode is enabled during configure.
bool config_logic_demo_enabled ()
{
  return (strcmp (DEMO, "yes") == 0);
}


// Returns the maximum number of parallel tasks to run.
int config_logic_max_parallel_tasks ()
{
  return PARALLEL_TASKS;
}


// Returns whether bare browser mode is enable during configure.
bool config_logic_bare_browser ()
{
  return (strcmp (BARE_BROWSER, "yes") == 0);
}


// The configured admin's username.
string config_logic_admin_username ()
{
  return ADMIN_USERNAME;
}


// The configured admin's password.
string config_logic_admin_password ()
{
  return ADMIN_PASSWORD;
}


// The configured admin's email.
string config_logic_admin_email ()
{
  return ADMIN_EMAIL;
}


// Replacement function for missing "stoi" on some platforms, like Cygwin and Android.
int my_stoi (const string& str, void * idx, int base)
{
#ifdef HAVE_STOI
  size_t * index = (size_t *) idx;
  return stoi (str, index, base);
#else
  char ** endptr = reinterpret_cast <char **> (idx);
  int i = strtol (str.c_str(), endptr, base);
  return i;
#endif
}


bool config_logic_paratext_enabled ()
{
#ifdef HAVE_PARATEXT
  return true;
#endif
  return false;
}


// Returns whether the external resources cache has been configured.
bool config_logic_external_resources_cache_configured ()
{
  return (strcmp (RESOURCES_PATH, "") != 0);
}


// Define the path to the shared external resources cache.
string config_logic_external_resources_cache_path ()
{
  return RESOURCES_PATH;
}


// Returns whether Windows has been enabled during configure.
bool config_logic_windows ()
{
  return (strcmp (WINDOWS, "yes") == 0);
}