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


#include <filter/git.h>
#include <filter/url.h>
#include <filter/string.h>
#include <database/logs.h>
#include <database/books.h>
#include <config.h>
#include <git2.h>
#include <bible/logic.h>
#include <locale/translate.h>


// This function returns the directory of the git repository belonging to $object.
string filter_git_directory (string object)
{
  return filter_url_create_root_path ("git", object);
}


string filter_git_check_error (int result)
{
  string msg;
  if (result != 0) {
    const git_error * error = giterr_last ();
    msg = error->message;
    Database_Logs::log (error->message);
  }
  return msg;
}


// Runs the equivalent of "git init".
bool filter_git_init (string directory)
{
  git_threads_init ();
  git_repository *repo = NULL;
  int result = git_repository_init (&repo, directory.c_str(), false);
  filter_git_check_error (result);
  git_repository_free (repo);
  git_threads_shutdown ();
  return (result == 0);
}


// This filter takes the Bible data as it is stored in Bibledit's database,
// and puts this information into the layout in books and chapters
// such as is used in Bibledit-Gtk into the $git folder.
// The $git is a git repository, and may contain other data as well.
// The filter focuses on reading the data in the git repository, and only writes to it if necessary,
// This speeds up the filter.
void filter_git_sync_bible_to_git (void * webserver_request, string bible, string repository)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  
  // First stage.
  // Read the chapters in the git repository,
  // and check if they occur in the database.
  // If a chapter is not in the database, remove it from the repository.
  vector <int> books = request->database_bibles()->getBooks (bible);
  vector <string> bookfiles = filter_url_scandir (repository);
  for (auto & bookname : bookfiles) {
    string path = filter_url_create_path (repository, bookname);
    if (filter_url_is_dir (path)) {
      int book = Database_Books::getIdFromEnglish (bookname);
      if (book) {
        if (in_array (book, books)) {
          // Book exists in the database: Check the chapters.
          vector <int> chapters = request->database_bibles()->getChapters (bible, book);
          vector <string> chapterfiles = filter_url_scandir (filter_url_create_path (repository, bookname));
          for (auto & chaptername : chapterfiles) {
            string path = filter_url_create_path (repository, bookname, chaptername);
            if (filter_url_is_dir (path)) {
              if (filter_string_is_numeric (chaptername)) {
                int chapter = convert_to_int (chaptername);
                string filename = filter_url_create_path (repository, bookname, chaptername, "data");
                if (file_exists (filename)) {
                  if (!in_array (chapter, chapters)) {
                    // Chapter does not exist in the database.
                    filter_url_rmdir (filter_url_create_path (repository, bookname, chaptername));
                  }
                }
              }
            }
          }
        } else {
          // Book does not exist in the database: Remove it from $git.
          filter_url_rmdir (filter_url_create_path (repository, bookname));
        }
      }
    }
  }
  
  // Second stage.
  // Read the books / chapters from the database,
  // and check if they occur in the repository, and the data matches.
  // If necessary, save the chapter to the repository.
  books = request->database_bibles()->getBooks (bible);
  for (auto & book : books) {
    string bookname = Database_Books::getEnglishFromId (book);
    string bookdir = filter_url_create_path (repository, bookname);
    if (!file_exists (bookdir)) filter_url_mkdir (bookdir);
    vector <int> chapters = request->database_bibles()->getChapters (bible, book);
    for (auto & chapter : chapters) {
      string chapterdir = filter_url_create_path (bookdir, to_string (chapter));;
      if (!file_exists (chapterdir)) filter_url_mkdir (chapterdir);
      string datafile = filter_url_create_path (chapterdir, "data");
      string contents = filter_url_file_get_contents (datafile);
      string usfm = request->database_bibles()->getChapter (bible, book, chapter);
      if (contents != usfm) filter_url_file_put_contents (datafile, usfm);
    }
  }
}


// This filter takes the Bible data as it is stored in the git $repository folder,
// and puts this information into Bibledit's database.
// The $repository is a git repository, and may contain other data as well.
// The filter focuses on reading the data in the git repository and the database,
// and only writes to the database if necessary,
// This speeds up the filter.
void filter_git_sync_git_to_bible (void * webserver_request, string repository, string bible)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;

  // Stage one:
  // Read the chapters in the git repository,
  // and check that they occur in the database.
  // If any does not occur, add the chapter to the database.
  // This stage does not check the contents of the chapters.
  vector <string> bookfiles = filter_url_scandir (repository);
  for (auto & bookname : bookfiles) {
    string bookpath = filter_url_create_path (repository, bookname);
    if (filter_url_is_dir (bookpath)) {
      int book = Database_Books::getIdFromEnglish (bookname);
      if (book) {
        // Check the chapters.
        vector <int> chapters = request->database_bibles()->getChapters (bible, book);
        vector <string> chapterfiles = filter_url_scandir (bookpath);
        for (auto & chapterfile : chapterfiles) {
          string chapterpath = filter_url_create_path (bookpath, chapterfile);
          if (filter_url_is_dir (chapterpath)) {
            if (filter_string_is_numeric (chapterfile)) {
              int chapter = convert_to_int (chapterfile);
              string filename = filter_url_create_path (chapterpath, "data");
              if (file_exists (filename)) {
                if (!in_array (chapter, chapters)) {
                  // Chapter does not exist in the database: Add it.
                  string usfm = filter_url_file_get_contents (filename);
                  Bible_Logic::storeChapter (bible, book, chapter, usfm);
                  // Log it.
                  string message = gettext("A translator added chapter") + " " + bible + " " + bookname + " " + chapterfile;
                  Database_Logs::log (message);
                }
              }
            }
          }
        }
      }
    }
  }
  
  
  // Stage two:
  // Read through the chapters in the database,
  // and check that they occur in the git folder.
  // If necessary, remove a chapter from the database.
  // If a chapter matches, check that the contents of the data in the git
  // folder and the contents in the database match.
  // If necessary, update the data in the database.
  vector <int> books = request->database_bibles()->getBooks (bible);
  for (auto & book : books) {
    string bookname = Database_Books::getEnglishFromId (book);
    string bookdir = filter_url_create_path (repository, bookname);
    if (file_exists (bookdir)) {
      vector <int> chapters = request->database_bibles()->getChapters (bible, book);
      for (auto & chapter : chapters) {
        string chapterdir = filter_url_create_path (bookdir, to_string (chapter));
        if (file_exists (chapterdir)) {
          string datafile = filter_url_create_path (chapterdir, "data");
          string contents = filter_url_file_get_contents (datafile);
          string usfm = request->database_bibles()->getChapter (bible, book, chapter);
          if (contents != usfm) {
            Bible_Logic::storeChapter (bible, book, chapter, contents);
            Database_Logs::log (gettext("A translator updated chapter") + " " + bible + " " + bookname + " " + to_string (chapter));
          }
        } else {
          Bible_Logic::deleteChapter (bible, book, chapter);
          Database_Logs::log (gettext("A translator deleted chapter") + " " + bible + " " + bookname + " " + to_string (chapter));
        }
      }
    } else {
      Bible_Logic::deleteBook (bible, book);
      Database_Logs::log (gettext("A translator deleted book") + " " + bible + " " + bookname);
    }
  }
}


// This filter takes one chapter of the Bible data as it is stored in the $git folder,
// and puts this information into Bibledit's database.
// The $git is a git repository, and may contain other data as well.
void filter_git_sync_git_chapter_to_bible (string repository, string bible, int book, int chapter)
{
  // Filename for the chapter.
  string bookname = Database_Books::getEnglishFromId (book);
  string filename = filter_url_create_path (repository, bookname, to_string (chapter), "data");
  
  if (file_exists (filename)) {
    
    // Store chapter in database.
    string usfm = filter_url_file_get_contents (filename);
    Bible_Logic::storeChapter (bible, book, chapter, usfm);
    Database_Logs::log (gettext("A collaborator updated") + " " + bible + " " + bookname + " " + to_string (chapter));
    
  } else {
    
    // Delete chapter from database.
    Bible_Logic::deleteChapter (bible, book, chapter);
    Database_Logs::log (gettext("A collaborator deleted chapter") + " " + bible + " " + bookname + " " + to_string (chapter));
    
  }
}


int cred_acquire_cb (git_cred **out, const char * url, const char * username_from_url, unsigned int allowed_types, void *payload)
{
  if (url) {};
  if (username_from_url) {};
  if (allowed_types) {};
  if (payload) {};
  
  char username[128] = {0};
  char password[128] = {0};
  
  return git_cred_userpass_plaintext_new (out, username, password);
}


// Returns true if the git repository at "url" is online.
bool filter_git_remote_read (string url, string & error) // Todo
{
  int result = 0;
  git_threads_init ();
  
  // Before running the actual test create an instance of a local repository.
  git_repository * repo = NULL;
  string repository_path = filter_url_tempfile ();
  filter_url_mkdir (repository_path);
  filter_git_init (repository_path);
  result = git_repository_open (&repo, repository_path.c_str());
  error = filter_git_check_error (result);
  
  // Create an instance of a remote from the URL.
  // The transport to use is detected from the URL
  git_remote * remote = NULL;
  if (result == 0) {
    result = git_remote_create (&remote, repo, "test", url.c_str ());
    error = filter_git_check_error (result);
  }

  // Callbacks.
  git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
  if (result == 0) {
    callbacks.credentials = cred_acquire_cb;
    git_remote_set_callbacks(remote, &callbacks);
  }
  
  // When connecting, the underlying code needs to know wether to push or fetch.
  if (result == 0) {
    result = git_remote_connect (remote, GIT_DIRECTION_FETCH);
    error = filter_git_check_error (result);
  }

  // Read from the repository.
  const git_remote_head **refs;
  size_t refs_len;
  if (result == 0) {
    result = git_remote_ls (&refs, &refs_len, remote);
    error = filter_git_check_error (result);
  }
  
  // Put output into the journal.
  if (result == 0) {
    for (size_t i = 0; i < refs_len; i++) {
      char oid [GIT_OID_HEXSZ + 1] = {0};
      git_oid_fmt (oid, &refs[i]->oid);
      string msg = "Reading remote repository: ";
      msg.append (oid);
      msg.append (" ");
      msg.append (refs[i]->name);
      Database_Logs::log (msg);
    }
  }
  
  // Free resources.
  if (remote) git_remote_free (remote);
  if (repo) git_repository_free (repo);
  git_threads_shutdown ();

  return (result == 0);
}
