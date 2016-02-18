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


#include <help/index.h>
#include <assets/view.h>
#include <assets/page.h>
#include <filter/roles.h>
#include <filter/url.h>
#include <filter/string.h>
#include <config/logic.h>
#include <locale/translate.h>
#include <assets/header.h>


string help_index_html (const string& url)
{
  string path = filter_url_basename (url);
  path.append (".html");
  path = filter_url_create_root_path ("help", path);
  return path;
}


bool help_index_url (const string& url)
{
  string folder = filter_url_dirname (url);
  if (folder != "help") return false;
  return file_exists (help_index_html (url));
}


bool help_index_acl (void * webserver_request, const string& url)
{
  if (url.empty()) {};
  return Filter_Roles::access_control (webserver_request, Filter_Roles::guest ());
}


string help_index (void * webserver_request, const string& url)
{
  string page;

  Assets_Header header = Assets_Header (translate("Help"), webserver_request);
  page = header.run ();

  Assets_View view;

  view.set_variable ("version", config_logic_version ());
  
  string linux_version = "1.0.xxx";
  string path = "/var/www/bibledit.org/linux";
  if (file_exists (path)) {
    vector <string> files = filter_url_scandir (path);
    if (!files.empty ()) {
      linux_version = files.back ();
      linux_version = filter_string_str_replace ("bibledit-", "", linux_version);
      linux_version = filter_string_str_replace (".tar.gz", "", linux_version);
    }
  }
  view.set_variable ("linuxversion", linux_version);

  page += view.render ("help", filter_url_basename (url));

  page += Assets_Page::footer ();

  return page;
}
