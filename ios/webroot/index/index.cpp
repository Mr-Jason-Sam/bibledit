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


#include <index/index.h>
#include <assets/view.h>
#include <assets/header.h>
#include <assets/page.h>
#include <filter/roles.h>
#include <filter/string.h>
#include <filter/url.h>
#include <locale/translate.h>
#include <edit/index.h>
#include <notes/index.h>
#include <resource/index.h>
#include <changes/changes.h>
#include <workbench/index.h>
#include <config/logic.h>
#include <session/login.h>


const char * index_index_url ()
{
  return "index/index";
}


bool index_index_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::guest ());
}


string index_index (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  
  Assets_Header header = Assets_Header ("Bibledit", webserver_request);
  
  // After a delay, the demo forwards to a set active workbench, when there's no active menu.
  if (config_logic_demo_enabled ()) {
    if (request->query.empty ()) {
      header.refresh (5, "/" + workbench_index_url ());
    }
  }
  
  // Normally a page does not show the extended main menu.
  // But the home page of Bibledit shows the main menu.
  if (request->query.count ("item") == 0) {
    request->query ["item"] = "main";
  }

  string page = header.run ();
  
  Assets_View view;

  page += view.render ("index", "index");
  page += Assets_Page::footer ();
  return page;
}
