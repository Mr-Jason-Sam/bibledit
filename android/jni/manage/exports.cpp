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


#include <manage/exports.h>
#include <assets/view.h>
#include <assets/page.h>
#include <filter/url.h>
#include <filter/roles.h>
#include <filter/string.h>
#include <dialog/list.h>
#include <dialog/entry.h>
#include <access/bible.h>
#include <locale/translate.h>
#include <export/logic.h>
#include <database/config/bible.h>


const char * manage_exports_url ()
{
  return "manage/exports";
}


bool manage_exports_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::manager ());
}


string manage_exports (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  
  
  string page;
  page = Assets_Page::header ("Export", webserver_request, "");
  Assets_View view = Assets_View ();
  
  
  if (request->query.count ("bible")) {
    string bible = request->query["bible"];
    if (bible == "") {
      Dialog_List dialog_list = Dialog_List ("exports", translate("Select a Bible"), "", "");
      vector <string> bibles = access_bible_bibles (webserver_request);
      for (auto bible : bibles) {
        // Select Bibles the user has write access to.
        if (access_bible_write (webserver_request, bible)) {
          dialog_list.add_row (bible, "bible", bible);
        }
      }
      page += dialog_list.run();
      return page;
    } else {
      request->database_config_user()->setBible (bible);
    }
  }
  
  
  string bible = access_bible_clamp (webserver_request, request->database_config_user()->getBible ());
  view.set_variable ("bible", bible);
  
  
  if (request->query.count ("remove")) {
    string directory = Export_Logic::bibleDirectory (bible);
    filter_url_rmdir (directory);
    view.set_variable ("success", translate("The export has been removed."));
  }
  
  
  if (request->query.count ("webtoggle")) {
    Database_Config_Bible::setExportWebDuringNight (bible, !Database_Config_Bible::getExportWebDuringNight (bible));
    view.set_variable ("success", translate("The setting for nightly export to Web format was updated."));
  }
  view.set_variable ("web", get_tick_box (Database_Config_Bible::getExportWebDuringNight (bible)));
  
  
  if (request->query.count ("webnow")) {
    Export_Logic::scheduleWeb (bible);
    Export_Logic::scheduleWebIndex (bible);
    view.set_variable ("success", translate("The Bible is being exported to Web format."));
  }
  
  
  if (request->query.count ("htmltoggle")) {
    Database_Config_Bible::setExportHtmlDuringNight (bible, !Database_Config_Bible::getExportHtmlDuringNight (bible));
    view.set_variable ("success", translate("The setting for nightly export to Html format was updated."));
  }
  view.set_variable ("html", get_tick_box (Database_Config_Bible::getExportHtmlDuringNight (bible)));
  
  
  if (request->query.count ("htmlnow")) {
    Export_Logic::scheduleHtml (bible);
    view.set_variable ("success", translate("The Bible is being exported to Html format."));
  }
  
  
  if (request->query.count ("usfmtoggle")) {
    Database_Config_Bible::setExportUsfmDuringNight (bible, !Database_Config_Bible::getExportUsfmDuringNight (bible));
    view.set_variable ("success", translate("The setting for nightly export to USFM format was updated."));
  }
  view.set_variable ("usfm", get_tick_box (Database_Config_Bible::getExportUsfmDuringNight (bible)));
 
  
  if (request->query.count ("usfmnow")) {
    Export_Logic::scheduleUsfm (bible);
    view.set_variable ("success", translate("The Bible is being exported to USFM format."));
  }

  
  if (request->query.count ("usfmsecuretoggle")) {
    Database_Config_Bible::setSecureUsfmExport (bible, !Database_Config_Bible::getSecureUsfmExport (bible));
    view.set_variable ("success", translate("The setting for securing the USFM export was updated."));
  }
  view.set_variable ("usfmsecure", get_tick_box (Database_Config_Bible::getSecureUsfmExport (bible)));

                     
  if (request->query.count ("texttoggle")) {
    Database_Config_Bible::setExportTextDuringNight (bible, !Database_Config_Bible::getExportTextDuringNight (bible));
    view.set_variable ("success", translate("The setting for nightly export to basic USFM format and text was updated."));
  }
  view.set_variable ("text", get_tick_box (Database_Config_Bible::getExportTextDuringNight (bible)));
  
  
  if (request->query.count ("textnow")) {
    Export_Logic::scheduleTextAndBasicUsfm (bible);
    view.set_variable ("success", translate("The Bible is being exported to basic USFM format and text."));
  }
                       
                     
  if (request->query.count ("odttoggle")) {
    Database_Config_Bible::setExportOdtDuringNight (bible, !Database_Config_Bible::getExportOdtDuringNight (bible));
    view.set_variable ("success", translate("The setting for nightly export to OpenDocument was updated."));
  }
  view.set_variable ("odt", get_tick_box (Database_Config_Bible::getExportOdtDuringNight (bible)));

  
  if (request->query.count ("odtnow")) {
    Export_Logic::scheduleOpenDocument (bible);
    view.set_variable ("success", translate("The Bible is being exported to OpenDocument format."));
  }

  
  if (request->query.count ("dropcapstoggle")) {
    Database_Config_Bible::setExportChapterDropCapsFrames (bible, !Database_Config_Bible::getExportChapterDropCapsFrames (bible));
  }
  view.set_variable ("dropcaps", get_tick_box (Database_Config_Bible::getExportChapterDropCapsFrames (bible)));

  
  if (request->query.count ("pagewidth")) {
    Dialog_Entry dialog_entry = Dialog_Entry ("exports", translate("Please enter a page width in millimeters"), Database_Config_Bible::getPageWidth (bible), "pagewidth", translate ("The width of A4 is 210 mm. The width of Letter is 216 mm."));
    page += dialog_entry.run ();
    return page;
  }
  if (request->post.count ("pagewidth")) {
    int value = convert_to_int (request->post["entry"]);
    if ((value >= 30) && (value <= 500)) {
      Database_Config_Bible::setPageWidth (bible, convert_to_string (value));
    }
  }
  view.set_variable ("pagewidth", Database_Config_Bible::getPageWidth (bible));

                     
  if (request->query.count ("pageheight")) {
    Dialog_Entry dialog_entry = Dialog_Entry ("exports", translate("Please enter a page height in millimeters"), Database_Config_Bible::getPageHeight (bible), "pageheight", translate ("The height of A4 is 297 mm. The width of Letter is 279 mm."));
    page += dialog_entry.run ();
    return page;
  }
  if (request->post.count ("pageheight")) {
    int value = convert_to_int (request->post["entry"]);
    if ((value >= 40) && (value <= 600)) {
      Database_Config_Bible::setPageHeight (bible, convert_to_string (value));
    }
  }
  view.set_variable ("pageheight", Database_Config_Bible::getPageHeight (bible));

  
  if (request->query.count ("innermargin")) {
    Dialog_Entry dialog_entry = Dialog_Entry ("exports", translate("Please enter an inner margin size in millimeters"), Database_Config_Bible::getInnerMargin (bible), "innermargin", "");
    page += dialog_entry.run ();
    return page;
  }
  if (request->post.count ("innermargin")) {
    int value = convert_to_int (request->post["entry"]);
    if ((value >= 0) && (value <= 100)) {
      Database_Config_Bible::setInnerMargin (bible, convert_to_string (value));
    }
  }
  view.set_variable ("innermargin", Database_Config_Bible::getInnerMargin (bible));


  if (request->query.count ("outermargin")) {
    Dialog_Entry dialog_entry = Dialog_Entry ("exports", translate("Please enter an outer margin size in millimeters"), Database_Config_Bible::getOuterMargin (bible), "outermargin", "");
    page += dialog_entry.run ();
    return page;
  }
  if (request->post.count ("outermargin")) {
    int value = convert_to_int (request->post["entry"]);
    if ((value >= 0) && (value <= 100)) {
      Database_Config_Bible::setOuterMargin (bible, convert_to_string (value));
    }
  }
  view.set_variable ("outermargin", Database_Config_Bible::getOuterMargin (bible));
  
  
  if (request->query.count ("topmargin")) {
    Dialog_Entry dialog_entry = Dialog_Entry ("exports", translate("Please enter an top margin size in millimeters"), Database_Config_Bible::getTopMargin (bible), "topmargin", "");
    page += dialog_entry.run ();
    return page;
  }
  if (request->post.count ("topmargin")) {
    int value = convert_to_int (request->post["entry"]);
    if ((value >= 0) && (value <= 100)) {
      Database_Config_Bible::setTopMargin (bible, convert_to_string (value));
    }
  }
  view.set_variable ("topmargin", Database_Config_Bible::getTopMargin (bible));


  if (request->query.count ("bottommargin")) {
    Dialog_Entry dialog_entry = Dialog_Entry ("exports", translate("Please enter an bottom margin size in millimeters"), Database_Config_Bible::getBottomMargin (bible), "bottommargin", "");
    page += dialog_entry.run ();
    return page;
  }
  if (request->post.count ("bottommargin")) {
    int value = convert_to_int (request->post["entry"]);
    if ((value >= 0) && (value <= 100)) {
      Database_Config_Bible::setBottomMargin (bible, convert_to_string (value));
    }
  }
  view.set_variable ("bottommargin", Database_Config_Bible::getBottomMargin (bible));
  

  if (request->query.count ("dateinheadertoggle")) {
    Database_Config_Bible::setDateInHeader (bible, !Database_Config_Bible::getDateInHeader (bible));
  }
  view.set_variable ("dateinheader", get_tick_box (Database_Config_Bible::getDateInHeader (bible)));
  
  
  if (request->query.count ("odtsecuretoggle")) {
    Database_Config_Bible::setSecureOdtExport (bible, !Database_Config_Bible::getSecureOdtExport (bible));
    view.set_variable ("success", translate("The setting for securing the OpenDocument export was updated."));
  }
  view.set_variable ("odtsecure", get_tick_box (Database_Config_Bible::getSecureOdtExport (bible)));
                     
                                          
  if (request->query.count ("infotoggle")) {
    Database_Config_Bible::setGenerateInfoDuringNight (bible, !Database_Config_Bible::getGenerateInfoDuringNight (bible));
    view.set_variable ("success", translate("The setting for nightly generation of info was updated."));
  }
  view.set_variable ("info", get_tick_box (Database_Config_Bible::getGenerateInfoDuringNight (bible)));
                   
  
  if (request->query.count ("infonow")) {
    Export_Logic::scheduleInfo (bible);
    view.set_variable ("success", translate("The info documents are being generated."));
  }
  
                       
  if (request->query.count ("eswordtoggle")) {
    Database_Config_Bible::setExportESwordDuringNight (bible, !Database_Config_Bible::getExportESwordDuringNight (bible));
    view.set_variable ("success", translate("The setting for nightly export to e-Sword format was updated."));
  }
  view.set_variable ("esword", get_tick_box (Database_Config_Bible::getExportESwordDuringNight (bible)));
                     
                                          
  if (request->query.count ("eswordnow")) {
    Export_Logic::scheduleESword (bible);
    view.set_variable ("success", translate("The Bible is being exported to e-Sword format."));
  }
  
                       
  if (request->query.count ("onlinebibletoggle")) {
    Database_Config_Bible::setExportOnlineBibleDuringNight (bible, !Database_Config_Bible::getExportOnlineBibleDuringNight (bible));
    view.set_variable ("success", translate("The setting for nightly export to Online Bible format was updated."));
  }
  view.set_variable ("onlinebible", get_tick_box (Database_Config_Bible::getExportOnlineBibleDuringNight (bible)));

                     
  if (request->query.count ("onlinebiblenow")) {
    Export_Logic::scheduleOnlineBible (bible);
    view.set_variable ("success", translate("The Bible is being exported to Online Bible format."));
  }
  
                       
  if (request->query.count ("sheet")) {
    string sheet = request->query["sheet"];
    if (sheet == "") {
      Dialog_List dialog_list = Dialog_List ("exports", translate("Would you like to use another stylesheet for the exports?"), "", "");
      vector <string> sheets = request->database_styles()->getSheets();
      for (auto sheet : sheets) {
        dialog_list.add_row (sheet, "sheet", sheet);
      }
      page += dialog_list.run ();
      return page;
    } else {
      Database_Config_Bible::setExportStylesheet (bible, sheet);
    }
  }
  view.set_variable ("stylesheet", Database_Config_Bible::getExportStylesheet (bible));
                     
                     
  if (request->post.count ("passwordsubmit")) {
    string password = request->post["passwordentry"];
    Database_Config_Bible::setExportPassword (bible, password);
    view.set_variable ("success", translate("The password for securing exports was saved."));
  }
  // If the password is needed, but not set, set a default password.
  if (Database_Config_Bible::getSecureUsfmExport (bible) || Database_Config_Bible::getSecureOdtExport (bible)) {
    if (Database_Config_Bible::getExportPassword (bible).empty ()) {
      Database_Config_Bible::setExportPassword (bible, "password");
    }
  }
  view.set_variable ("password", Database_Config_Bible::getExportPassword (bible));
                     

  page += view.render ("manage", "exports");
  page += Assets_Page::footer ();
  return page;
}