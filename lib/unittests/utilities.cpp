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


#include <unittests/utilities.h>
#include <filter/string.h>
#include <filter/url.h>


string testing_directory;
int error_count;


// Puts a fresh and clean copy of Bibledit into the sandbox in the testing directory.
void refresh_sandbox (bool displayjournal)
{
  // Display any old journal entries.
  if (displayjournal) {
    string directory = filter_url_create_path (testing_directory, "logbook");
    vector <string> files = filter_url_scandir (directory);
    for (unsigned int i = 0; i < files.size (); i++) {
      if (files [i] == "gitflag") continue;
      string contents = filter_url_file_get_contents (filter_url_create_path (directory, files [i]));
      cout << contents << endl;
    }
  }
  
  // Refresh.
  string command = "rsync . -a --delete " + testing_directory;
  int status = system (command.c_str());
  if (status != 0) {
    cout << "Error while running " + command << endl;
    exit (status);
  }
}


void error_message (int line, string func, string desired, string actual)
{
  cout << "Line number:    " << line << endl;
  cout << "Function:       " << func << endl;
  cout << "Desired result: " << desired << endl;
  cout << "Actual result:  " << actual << endl;
  cout << endl;
  error_count++;
}


void evaluate (int line, string func, string desired, string actual)
{
  if (desired != actual) error_message (line, func, desired, actual);
}


void evaluate (int line, string func, int desired, int actual)
{
  if (desired != actual) error_message (line, func, convert_to_string (desired), convert_to_string (actual));
}


void evaluate (int line, string func, unsigned int desired, unsigned int actual)
{
  if (desired != actual) error_message (line, func, convert_to_string (desired), convert_to_string (actual));
}


void evaluate (int line, string func, bool desired, bool actual)
{
  if (desired != actual) error_message (line, func, desired ? "true" : "false", actual ? "true" : "false");
}


void evaluate (int line, string func, vector <string> desired, vector <string> actual)
{
  if (desired.size() != actual.size ()) {
    error_message (line, func, convert_to_string ((int)desired.size ()), convert_to_string ((int)actual.size()) + " size mismatch");
    return;
  }
  for (unsigned int i = 0; i < desired.size (); i++) {
    if (desired[i] != actual[i]) error_message (line, func, desired[i], actual[i] + " mismatch at offset " + convert_to_string (i));
  }
}


void evaluate (int line, string func, vector <int> desired, vector <int> actual)
{
  if (desired.size() != actual.size ()) {
    error_message (line, func, convert_to_string ((int)desired.size ()), convert_to_string ((int)actual.size()) + " size mismatch");
    return;
  }
  for (unsigned int i = 0; i < desired.size (); i++) {
    if (desired[i] != actual[i]) error_message (line, func, convert_to_string (desired[i]), convert_to_string (actual[i]) + " mismatch at offset " + convert_to_string (i));
  }
}


void evaluate (int line, string func, map <int, string> desired, map <int, string> actual)
{
  if (desired.size() != actual.size ()) {
    error_message (line, func, convert_to_string ((int)desired.size ()), convert_to_string ((int)actual.size()) + " size mismatch");
    return;
  }
  auto desirediterator = desired.begin ();
  auto actualiterator = actual.begin ();
  for (auto iterator = desired.begin(); iterator != desired.end(); iterator++) {
    evaluate (line, func, desirediterator->first, actualiterator->first);
    evaluate (line, func, desirediterator->second, actualiterator->second);
    desirediterator++;
    actualiterator++;
  }
}


void evaluate (int line, string func, map <string, string> desired, map <string, string> actual)
{
  if (desired.size() != actual.size ()) {
    error_message (line, func, convert_to_string ((int)desired.size ()), convert_to_string ((int)actual.size()) + " size mismatch");
    return;
  }
  auto desirediterator = desired.begin ();
  auto actualiterator = actual.begin ();
  for (auto iterator = desired.begin(); iterator != desired.end(); iterator++) {
    evaluate (line, func, desirediterator->first, actualiterator->first);
    evaluate (line, func, desirediterator->second, actualiterator->second);
    desirediterator++;
    actualiterator++;
  }
}

