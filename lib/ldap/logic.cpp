/*
 Copyright (©) 2003-2016 Teus Benschop.
 
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


#include <ldap/logic.h>
#include <filter/string.h>
#include <filter/url.h>
#include <filter/shell.h>
#include <filter/roles.h>
#include <database/logs.h>


/*
 
 Public LDAP test server:
 http://www.forumsys.com/en/tutorials/integration-how-to/ldap/online-ldap-test-server/
 
 Examples of search queries against the test server:
 
 ldapsearch -D "cn=read-only-admin,dc=example,dc=com" -w password -h ldap.forumsys.com -p 389 -b "dc=example,dc=com" -s sub "(objectclass=*)"
 
 ldapsearch -D "uid=boyle,dc=example,dc=com" -w password -h ldap.forumsys.com -p 389
 
 ldapsearch -h ldap.forumsys.com -p 389 -w password -D "cn=read-only-admin,dc=example,dc=com" -b "dc=example,dc=com" -s sub "(objectclass=*)"
 
 ldapsearch -H ldap://ldap.forumsys.com -D "cn=read-only-admin,dc=example,dc=com" -w password -b "dc=example,dc=com" -s sub "(objectclass=*)"

 ldapsearch -H ldap://ldap.forumsys.com -D "uid=boyle,dc=example,dc=com" -w password -b "dc=example,dc=com" -s sub "(uid=boyle)"

 ldapsearch -H ldap://192.168.2.12 -D "cn=admin,dc=test,dc=com" -w openldap -b "dc=test,dc=com" -s sub "(objectClass=*)"
 
 ldapsearch -H ldap://192.168.2.12 -D "cn=user,dc=test,dc=com" -w benschop -b "dc=test,dc=com" -s sub "(cn=user)"

 */


// These global variables contain the settings for the OpenLDAP server
// to query for user credentials.
string ldap_logic_uri;
string ldap_logic_binddn;
string ldap_logic_basedn;
string ldap_logic_scope;
string ldap_logic_filter;
string ldap_logic_role;


// Initialize the configuration for accessing an OpenLDAP server.
void ldap_logic_initialize ()
{
  // Check if the OpenLDAP configuration file exists.
  string path = filter_url_create_root_path ("config", "ldap.conf");
  if (file_or_dir_exists (path)) {
    // Parse the configuration file.
    string contents = filter_url_file_get_contents (path);
    vector <string> lines = filter_string_explode (contents, '\n');
    for (auto line : lines) {
      line = filter_string_trim (line);
      if (line.empty ()) continue;
      if (line.substr (0, 1) == "#") continue;
      size_t pos = line.find ("=");
      string key = filter_string_trim (line.substr (0, pos));
      line.erase (0, ++pos);
      line = filter_string_trim (line);
      if (key == "uri"   ) ldap_logic_uri    = line;
      if (key == "binddn") ldap_logic_binddn = line;
      if (key == "basedn") ldap_logic_basedn = line;
      if (key == "scope" ) ldap_logic_scope  = line;
      if (key == "filter") ldap_logic_filter = line;
      if (key == "role"  ) ldap_logic_role   = line;
    }
    // Log the results.
    if (ldap_logic_on (true)) {
      Database_Logs::log ("Using LDAP for authentication");
    }
  }
}


// Clear LDAP configuration.
void ldap_logic_clear ()
{
  ldap_logic_uri.clear ();
  ldap_logic_binddn.clear ();
  ldap_logic_basedn.clear ();
  ldap_logic_scope.clear ();
  ldap_logic_filter.clear ();
  ldap_logic_role.clear ();
}


// Returns true if authentication through OpenLDAP is on.
bool ldap_logic_on (bool log)
{
  if (ldap_logic_uri.empty ()) {
    if (log) Database_Logs::log ("LDAP server configuration lacks the URI");
    return false;
  }
  if (ldap_logic_binddn.empty ()) {
    if (log) Database_Logs::log ("LDAP server configuration lacks the bind dn");
    return false;
  }
  if (ldap_logic_basedn.empty ()) {
    if (log) Database_Logs::log ("LDAP server configuration lacks the base dn");
    return false;
  }
  if (ldap_logic_scope.empty ()) {
    if (log) Database_Logs::log ("LDAP server configuration lacks the scope");
    return false;
  }
  if (ldap_logic_filter.empty ()) {
    if (log) Database_Logs::log ("LDAP server configuration lacks the search filter");
    return false;
  }
  if (ldap_logic_role.empty ()) {
    if (log) Database_Logs::log ("LDAP server configuration lacks the role field");
    return false;
  }
  return true;
}


// Queries the LDAP server with credentials $user and $password.
// Parameter $access indicates whether the credentials have access to the server.
// Parameter $mail returns the email address.
// Parameter $role returns the user's role.
// If the query was done successfully, the function returns true.
bool ldap_logic_get (string user, string password, bool & access, string & email, int & role, bool log)
{
  // Initialize result values for the caller.
  access = false;
  email.clear ();
  role = Filter_Roles::guest ();
  
  // Insert the user name where appropriate.
  string binddn = filter_string_str_replace ("[user]", user, ldap_logic_binddn);
  string filter = filter_string_str_replace ("[user]", user, ldap_logic_filter);
  
  // Query the LDAP server.
  string output;
  int result = filter_shell_vfork (output, "ldapsearch",
                                   "-H", ldap_logic_uri.c_str (),
                                   "-D", binddn.c_str (),
                                   "-w", password.c_str (),
                                   "-b", ldap_logic_basedn.c_str (),
                                   "-s", ldap_logic_scope.c_str(),
                                   filter.c_str());
  
  // Logging.
  if (log) {
    string command = "ldapsearch -H " + ldap_logic_uri + " -D " + binddn + " -w " + password + " -b " + ldap_logic_basedn + " -s " + ldap_logic_scope + " " + filter;
    Database_Logs::log ("LDAP query\n" + command + "\n" + output, Filter_Roles::admin ());
  }
  
  
  // Check on invalid credentials.
  if (result == 12544) {
    return true;
  }
  
  // Parse server response.
  if (result == 0) {
    access = true;
    vector <string> lines = filter_string_explode (output, '\n');
    for (auto & line : lines) {
      if (line.find ("mail:") == 0) {
        email = filter_string_trim (line.substr (5));
      }
      if (line.find (ldap_logic_role + ":") == 0) {
        string fragment = unicode_string_casefold (filter_string_trim (line.substr (3)));
        for (int r = Filter_Roles::lowest (); r <= Filter_Roles::highest (); r++) {
          if (fragment.find (unicode_string_casefold (Filter_Roles::english (r))) != string::npos) {
            role = r;
          }
        }
      }
    }
    return true;
  }
  
  // Communication failure or another error.
  return false;
}
