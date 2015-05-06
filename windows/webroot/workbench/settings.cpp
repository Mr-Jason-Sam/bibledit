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


#include <workbench/settings.h>
#include <assets/view.h>
#include <assets/page.h>
#include <assets/header.h>
#include <filter/roles.h>
#include <filter/string.h>
#include <webserver/request.h>
#include <locale/translate.h>
#include <database/config/general.h>
#include <workbench/logic.h>
#include <workbench/index.h>
#include <dialog/yes.h>
#include <filter/url.h>


string workbench_settings_url ()
{
  return "workbench/settings";
}


bool workbench_settings_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::consultant ());
}


string workbench_settings (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  
  string name = request->query ["name"];
  request->database_config_user()->setActiveWorkbench (name);
  
  if (request->query.count ("preset")) {
    int preset = stoi (request->query ["preset"]);
    workbenchSetURLs (request, workbenchDefaultURLs (preset));
    workbenchSetWidths (request, workbenchDefaultWidths (preset));
    workbenchSetHeights (request, workbenchDefaultHeights (preset));
  }
  
  if (request->post.count ("save")) {
    map <int, string> urls;
    map <int, string> widths;
    map <int, string> row_heights;
    int to14 = 0;
    int to2 = 0;
    for (int row = 1; row <= 3; row++) {
      for (int column = 1; column <= 5; column++) {
        string key = convert_to_string (row) + convert_to_string (column);
        urls [to14] = request->post ["url" + key];
        widths [to14] = request->post ["width" + key];
        to14++;
      }
      string key = convert_to_string (row);
      row_heights [to2] = request->post ["height" + key];
      to2++;
    }
    workbenchSetURLs (request, urls);
    workbenchSetWidths (request, widths);
    workbenchSetHeights (request, row_heights);
    redirect_browser (request, workbench_index_url ());
    return "";
  }
  
  string page;
  
  Assets_Header header = Assets_Header (translate("Edit workbench"), request);
  page = header.run ();
  
  Assets_View view = Assets_View ();
  
  map <int, string> urls = workbenchGetURLs (request, false);
  map <int, string> widths = workbenchGetWidths (request);
  for (auto & element : urls) {
    int key = element.first;
    int row = round (key / 5) + 1;
    int column = key % 5 + 1;
    string variable = "url" + convert_to_string (row) + convert_to_string (column);
    view.set_variable (variable, urls[key]);
    variable = "width" + convert_to_string (row) + convert_to_string (column);
    view.set_variable (variable, widths[key]);
  }
  
  map <int, string> row_heights = workbenchGetHeights (request);
  for (auto & element : row_heights) {
    int key = element.first;
    int row = key + 1;
    string variable = "height" + convert_to_string (row);
    view.set_variable (variable, row_heights [key]);
  }
  
  view.set_variable ("name", name);
  
  page += view.render ("workbench", "settings");
  
  page += Assets_Page::footer ();
  
  return page;
}
