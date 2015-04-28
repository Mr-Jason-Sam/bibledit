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


#include <changes/modifications.h>
#include <filter/string.h>
#include <filter/url.h>
#include <filter/roles.h>
#include <filter/usfm.h>
#include <filter/text.h>
#include <filter/diff.h>
#include <filter/passage.h>
#include <filter/date.h>
#include <html/text.h>
#include <text/text.h>
#include <database/logs.h>
#include <database/modifications.h>
#include <database/mail.h>
#include <database/config/general.h>
#include <database/config/bible.h>
#include <webserver/request.h>
#include <locale/translate.h>
#include <client/logic.h>
#include <access/bible.h>


// Helper function.
void changes_process_identifiers (Webserver_Request * request,
                                  string user,
                                  string bible,
                                  int book, int chapter,
                                  int oldId, int newId,
                                  string & email)
{
  if (oldId != 0) {
    Database_Modifications database_modifications = Database_Modifications ();
    string stylesheet = Database_Config_Bible::getExportStylesheet (bible);
    Database_Modifications_Text old_chapter_text = database_modifications.getUserChapter (user, bible, book, chapter, oldId);
    string old_chapter_usfm = old_chapter_text.oldtext;
    Database_Modifications_Text new_chapter_text = database_modifications.getUserChapter (user, bible, book, chapter, newId);
    string new_chapter_usfm = new_chapter_text.newtext;
    //int timestamp = database_modifications.getUserTimestamp (user, bible, book, chapter, newId);
    vector <int> old_verse_numbers = usfm_get_verse_numbers (old_chapter_usfm);
    vector <int> new_verse_numbers = usfm_get_verse_numbers (new_chapter_usfm);
    vector <int> verses = old_verse_numbers;
    verses.insert (verses.end (), new_verse_numbers.begin (), new_verse_numbers.end ());
    verses = array_unique (verses);
    sort (verses.begin(), verses.end());
    for (auto verse : verses) {
      string old_verse_usfm = usfm_get_verse_text (old_chapter_usfm, verse);
      string new_verse_usfm = usfm_get_verse_text (new_chapter_usfm, verse);
      if (old_verse_usfm != new_verse_usfm) {
        Filter_Text filter_text_old = Filter_Text (bible);
        Filter_Text filter_text_new = Filter_Text (bible);
        filter_text_old.html_text_standard = new Html_Text (translate("Bible"));
        filter_text_new.html_text_standard = new Html_Text (translate("Bible"));
        filter_text_old.text_text = new Text_Text ();
        filter_text_new.text_text = new Text_Text ();
        filter_text_old.addUsfmCode (old_verse_usfm);
        filter_text_new.addUsfmCode (new_verse_usfm);
        filter_text_old.run (stylesheet);
        filter_text_new.run (stylesheet);
        string old_html = filter_text_old.html_text_standard->getInnerHtml ();
        string new_html = filter_text_new.html_text_standard->getInnerHtml ();
        string old_text = filter_text_old.text_text->get ();
        string new_text = filter_text_new.text_text->get ();
        if (old_text != new_text) {
          string modification = filter_diff_diff (old_text, new_text);
          email += "<div>";
          email += filter_passage_display (book, chapter, convert_to_string (verse));
          email += " ";
          email += modification;
          email += "</div>";
          if (request->database_config_user()->getUserUserChangesNotificationsOnline (user)) {
            database_modifications.recordNotification ({user}, "☺", bible, book, chapter, verse, old_html, modification, new_html);
          }
        }
      }
    }
  }
}


void changes_modifications ()
{
  Database_Logs::log ("Change notifications: Generating", Filter_Roles::translator ());

  
  // Data objects.
  Webserver_Request request;
  Database_Modifications database_modifications = Database_Modifications ();
  Database_Mail database_mail = Database_Mail (&request);


  // Recreate modifications database.
  database_modifications.erase ();
  database_modifications.create ();
  
  
  // Create online change notifications for users who made changes in Bibles
  // through the web editor or through a client.
  // It runs before the team changes.
  // This produces the desired order of the notifications in the GUI.
  vector <string> users = database_modifications.getUserUsernames ();
  if (!users.empty ()) Database_Logs::log ("Change notifications: Per user", Filter_Roles::translator ());
  for (auto user : users) {
    
    // Go through the Bibles changed by the current user.
    vector <string> bibles = database_modifications.getUserBibles (user);
    for (auto bible : bibles) {
      
      // Body of the email to be sent.
      string email = "<p>" + translate("You have entered the changes below in the Bible editor.") + " " + translate ("You may check if it made its way into the Bible text.") + "</p>";
      
      // Go through the books in that Bible.
      vector <int> books = database_modifications.getUserBooks (user, bible);
      for (auto book : books) {
        
        // Go through the chapters in that book.
        vector <int> chapters = database_modifications.getUserChapters (user, bible, book);
        for (auto chapter : chapters) {
          
          // Get the sets of identifiers for that chapter, and set some variables.
          vector <Database_Modifications_Id> IdSets = database_modifications.getUserIdentifiers (user, bible, book, chapter);
          int referenceOldId = 0;
          int referenceNewId = 0;
          int newId = 0;
          int lastNewId = 0;
          bool restart = true;
          
          // Go through the sets of identifiers.
          for (auto IdSet : IdSets) {
            
            int oldId = IdSet.oldid;
            newId = IdSet.newid;
            
            if (restart) {
              changes_process_identifiers (&request, user, bible, book, chapter, referenceNewId, newId, email);
              referenceOldId = oldId;
              referenceNewId = newId;
              lastNewId = newId;
              restart = false;
              continue;
            }
            
            if (oldId == lastNewId) {
              lastNewId = newId;
            } else {
              restart = true;
            }
          }
          
          // Process the last set of identifiers.
          changes_process_identifiers (&request, user, bible, book, chapter, referenceNewId, newId, email);
          
        }
      }
      
      // Send the user email with the user's personal changes if the user opted to receive it.
      if (request.database_config_user()->getUserUserChangesNotification (user)) {
        string subject = translate("Changes you entered in") + " " + bible;
        if (!client_logic_client_enabled ()) database_mail.send (user, subject, email);
      }
      email.clear ();
      
    }
    
    // Clear the user's changes in the database.
    database_modifications.clearUserUser (user);
    
  }
  
  
  // Generate the notifications, online and by email,
  // for the changes in the Bibles accepted by the team since the previous notifications were generated.
  vector <string> bibles = database_modifications.getTeamDiffBibles ();
  for (auto bible : bibles) {
    
    
    string stylesheet = Database_Config_Bible::getExportStylesheet (bible);
    
    
    vector <string> changeNotificationUsers;
    vector <string> users = request.database_users ()->getUsers ();
    for (auto user : users) {
      if (access_bible_read (&request, bible, user)) {
        if (request.database_config_user()->getUserGenerateChangeNotifications (user)) {
          changeNotificationUsers.push_back (user);
        }
      }
    }
    users.clear ();
    
    
    // The number of changes processed so far for this Bible.
    int processedChangesCount = 0;
    
    
    // The files get stored at http://site.org:<port>/revisions/<Bible>/<date>
    int seconds = filter_date_seconds_since_epoch ();
    string timepath;
    timepath.append (convert_to_string (filter_date_numerical_year (seconds)));
    timepath.append ("-");
    timepath.append (filter_string_fill (convert_to_string (filter_date_numerical_month (seconds)), 2, '0'));
    timepath.append ("-");
    timepath.append (filter_string_fill (convert_to_string (filter_date_numerical_month_day (seconds)), 2, '0'));
    timepath.append (" ");
    timepath.append (filter_string_fill (convert_to_string (filter_date_numerical_hour (seconds)), 2, '0'));
    timepath.append (":");
    timepath.append (filter_string_fill (convert_to_string (filter_date_numerical_minute (seconds)), 2, '0'));
    timepath.append (":");
    timepath.append (filter_string_fill (convert_to_string (filter_date_numerical_second (seconds)), 2, '0'));
    string directory = filter_url_create_root_path ("revisions", bible, timepath);
    filter_url_mkdir (directory);
    
    
    // Produce the USFM and html files.
    filter_diff_produce_verse_level (bible, directory);
    
    
    // Create online page with changed verses.
    string versesoutputfile = filter_url_create_path (directory, "changed_verses.html");
    filter_diff_run_file (filter_url_create_path (directory, "verses_old.txt"), filter_url_create_path (directory, "verses_new.txt"), versesoutputfile);
    
    
    // Email users.
    string subject = translate("Recent changes") + " " + bible;
    string emailBody = filter_url_file_get_contents (versesoutputfile);
    users = request.database_users ()->getUsers ();
    for (auto user : users) {
      if (request.database_config_user()->getUserBibleChangesNotification (user)) {
        if (access_bible_read (&request, bible, user)) {
          if (!client_logic_client_enabled ()) database_mail.send (user, subject, emailBody);
        }
      }
    }
    
    
    // Generate the online change notifications.
    vector <int> books = database_modifications.getTeamDiffBooks (bible);
    for (auto book : books) {
      vector <int> chapters = database_modifications.getTeamDiffChapters (bible, book);
      for (auto chapter : chapters) {
        Database_Logs::log ("Change notifications: " + bible + " " + filter_passage_display (book, chapter, ""), Filter_Roles::translator ());
        string old_chapter_usfm = database_modifications.getTeamDiff (bible, book, chapter);
        string new_chapter_usfm = request.database_bibles()->getChapter (bible, book, chapter);
        vector <int> old_verse_numbers = usfm_get_verse_numbers (old_chapter_usfm);
        vector <int> new_verse_numbers = usfm_get_verse_numbers (new_chapter_usfm);
        vector <int> verses = old_verse_numbers;
        verses.insert (verses.end (), new_verse_numbers.begin (), new_verse_numbers.end ());
        verses = array_unique (verses);
        sort (verses.begin (), verses.end());
        for (auto verse : verses) {
          string old_verse_usfm = usfm_get_verse_text (old_chapter_usfm, verse);
          string new_verse_usfm = usfm_get_verse_text (new_chapter_usfm, verse);
          if (old_verse_usfm != new_verse_usfm) {
            processedChangesCount++;
            // In case of too many change notifications, processing them would take too much time, so take a few shortcuts.
            string old_html = "<p>" + old_verse_usfm + "</p>";
            string new_html = "<p>" + new_verse_usfm + "</p>";
            string old_text = old_verse_usfm;
            string new_text = new_verse_usfm;
            if (processedChangesCount < 800) {
              Filter_Text filter_text_old = Filter_Text (bible);
              Filter_Text filter_text_new = Filter_Text (bible);
              filter_text_old.html_text_standard = new Html_Text (translate("Bible"));
              filter_text_new.html_text_standard = new Html_Text (translate("Bible"));
              filter_text_old.text_text = new Text_Text ();
              filter_text_new.text_text = new Text_Text ();
              filter_text_old.addUsfmCode (old_verse_usfm);
              filter_text_new.addUsfmCode (new_verse_usfm);
              filter_text_old.run (stylesheet);
              filter_text_new.run (stylesheet);
              old_html = filter_text_old.html_text_standard->getInnerHtml ();
              new_html = filter_text_new.html_text_standard->getInnerHtml ();
              old_text = filter_text_old.text_text->get ();
              new_text = filter_text_new.text_text->get ();
            }
            if (old_text != new_text) {
              string modification = filter_diff_diff (old_text, new_text);
              database_modifications.recordNotification (changeNotificationUsers, "♺", bible, book, chapter, verse, old_html, modification, new_html);
            }
          }
        }
        // Delete the diff data for this chapter, for two reasons:
        // 1. New diffs for this chapter can be stored straightaway.
        // 2. In case of large amounts of diff data, and this function gets killed,
        //    then the next time it runs again, it will continue from where it was killed.
        database_modifications.deleteTeamDiffChapter (bible, book, chapter);
      }
    }
  }
  
  
  // Index the data and remove expired notifications.
  Database_Logs::log ("Change notifications: Indexing", Filter_Roles::translator ());
  database_modifications.indexTrimAllNotifications ();
  
  
  Database_Logs::log ("Change notifications: Ready", Filter_Roles::translator ());
}
