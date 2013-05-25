<?php
/**
* @package bibledit
*/
/*
 ** Copyright (©) 2003-2013 Teus Benschop.
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 3 of the License, or
 ** (at your option) any later version.
 **  
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **  
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **  
 */


class Database_Notes
{
  private static $instance;
  private $standard_statuses = array ("New", "Pending", "In progress", "Done", "Reopened");
  private $standard_severities = array (0 => "Wish", 1 => "Minor", 2 => "Normal", 3 => "Important", 4 => "Major", 5 => "Critical");

  private function __construct() 
  {
  } 

  public static function getInstance() 
  {
    if ( empty( self::$instance ) ) {
      self::$instance = new Database_Notes();
    }
    // Enter the standard statuses in the list of translatable strings.
    if (false) {
      gettext ("New");
      gettext ("Pending");
      gettext ("In progress");
      gettext ("Done");
      gettext ("Reopened");
    }
    // Enter the standard severities in the list of translatable strings.
    if (false) {
      gettext ("Wish");
      gettext ("Minor");
      gettext ("Normal");
      gettext ("Important");
      gettext ("Major");
      gettext ("Critical");
    }
    return self::$instance;
  }


  public function optimize () {
    $database_instance = Database_Instance::getInstance();
    $database_instance->runQuery ("OPTIMIZE TABLE notes;");
  }


  public function identifierExists ($identifier)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $query = "SELECT identifier FROM notes WHERE identifier = $identifier;";
    $result = $server->runQuery ($query);
    return ($result->num_rows > 0);
  }

  
  /**
  * Update a note's $identifier.
  * $new_identifier is the value given to the note identifier by $identifier.
  */
  public function setIdentifier ($identifier, $new_identifier)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $new_identifier = Database_SQLInjection::no ($new_identifier);
    $query = "UPDATE notes SET identifier = $new_identifier WHERE identifier = $identifier;";
    $server->runQuery ($query);
  }
  
  
  public function getNewUniqueIdentifier ()
  {
    do {
      $identifier = rand (100000000, 999999999);
    } while ($this->identifierExists ($identifier));
    return $identifier;
  }
  
  
  public function getIdentifiers ()
  {
    $server = Database_Instance::getInstance ();
    $identifiers = array ();
    $query = "SELECT identifier FROM notes;";
    $result = $server->runQuery ($query);
    for ($i = 0; $i < $result->num_rows; $i++) {
      $row = $result->fetch_row();
      $identifiers [] = $row[0];
    }
    return $identifiers;
  }
  

  private function assembleContents ($identifier, $contents)
  {
    $new_contents = "";
    if (is_numeric ($identifier)) {
      $new_contents = $this->getContents ($identifier);
    }
    if (version_compare(PHP_VERSION, '5.2.0', '>=')) {
      $datetime = new DateTime();
      Filter_Datetime::user_zone ($datetime);
      $datetime = $datetime->format(DATE_RSS);
    } else {
      $datetime = strftime ("%a, %e %b %G %H:%M:%S %z");
    }
    $session_logic = Session_Logic::getInstance();
    $user = $session_logic->currentUser ();
    $new_contents .= "<p>$user ($datetime):</p>\n";
    $lines = explode ("\n", $contents);
    foreach ($lines as $line) {
      $line = trim ($line);
      $new_contents .= "<p>$line</p>\n";
    }
    return $new_contents;
  }


  /**
    * Store a new note into the database.
    * $bible: The notes's Bible.
    * $book, $chapter, $verse: The note's passage.
    * $summary: The note's summary.
    * $contents: The note's contents.
    * $raw: Import $contents as it is. Useful for import from Bibledit-Gtk.
    */  
  public function storeNewNote ($bible, $book, $chapter, $verse, $summary, $contents, $raw)
  {
    // Store new default note into the database.
    $server = Database_Instance::getInstance ();
    $identifier = $this->getNewUniqueIdentifier ();
    $bible = Database_SQLInjection::no ($bible);
    $passage = Database_SQLInjection::no ($this->encodePassage ($book, $chapter, $verse));
    // If the $summary is not given, take the first line of the $contents as the $summary.
    if ($summary == "") {
      // The wysiwyg editor, jsysiwyg, does not put new lines at each line, but instead <div>s. Handle these also.
      $summary = str_replace ("<", "\n", $contents);
      $summary = explode ("\n", $summary);
      $summary = $summary[0];
    }
    $summary = Database_SQLInjection::no ($summary);
    if (!$raw) $contents = $this->assembleContents ($identifier, $contents);
    $contents = Database_SQLInjection::no ($contents);
    if (($contents == "") && ($summary == "")) return;
    $query = "INSERT INTO notes VALUES (NULL, $identifier, 0, '', '', '$bible', '$passage', 'New', 2, 0, '$summary', '$contents', NULL, NULL)";
    $server->runQuery ($query);
    $this-> updateSearchFields ($identifier);
    $this->noteEditedActions ($identifier);
    // Return this new note´s identifier.
    return $identifier;
  }


  /**
  * Returns an array of note identifiers selected.
  * If $limit is non-NULL, it indicates the starting limit for the selection.
  * $book, $chapter, $verse, $passage_selector: These are related and can limit the selection.
  * $edit_selector: Optionally constrains selection based on modification time.
  * $non_edit_selector: Optionally constrains selection based on modification time.
  * $status_selector: Optionally constrains selection based on note status.
  * $bible_selector: Optionally constrains the selection, based on the note's Bible.
  * $assignment_selector: Optionally constrains the selection based on a note being assigned to somebody.
  * $subscription_selector: Optionally limits the selection based on a note's subscription.
  * $severity_selector: Optionally limits the selection, based on a note's severity.
  * $text_selector: Optionally limits the selection to notes that contains certain text. Used for searching notes.
  * $search_text: Works with $text_selector, contains the text to search for.
  * $userlevel: if 0, it takes the user's level from the current user, else it takes the level passed in the variable $userlevel itself.
  */
  public function selectNotes ($bible, $book, $chapter, $verse, $passage_selector, $edit_selector, $non_edit_selector, $status_selector, $bible_selector, $assignment_selector, $subscription_selector, $severity_selector, $text_selector, $search_text, $limit, $userlevel)
  {
    $session_logic = Session_Logic::getInstance ();
    if ($userlevel == 0)  $userlevel = $session_logic->currentLevel ();
    $username = $session_logic->currentUser ();
    $identifiers = array ();
    $server = Database_Instance::getInstance ();
    // SQL SELECT statement.
    $query = Filter_Sql::notesSelectIdentifier ();
    // SQL optional fulltext search statement sorted on relevance.
    if ($text_selector == 1) {
      $query .= Filter_Sql::notesOptionalFulltextSearchRelevanceStatement ($search_text);
    }
    // SQL FROM ... WHERE statement.
    $query .= Filter_Sql::notesFromWhereStatement ();
    // SQL privacy statement.
    $query .= Filter_Sql::notesConsiderPrivacy ($userlevel);
    // Consider passage selector.
    switch ($passage_selector) {
      case 0:
        // Select notes that refer to the current verse.
        // It means that the book, the chapter, and the verse, should match.
        $passage = $this->encodePassage ($book, $chapter, $verse);
        $query .= " AND passage LIKE '%$passage%' ";
        break;
      case 1:
        // Select notes that refer to the current chapter.
        // It means that the book and the chapter should match.
        $passage = $this->encodePassage ($book, $chapter, NULL);
        $query .= " AND passage LIKE '%$passage%' ";
        break;
      case 2:
        // Select notes that refer to the current book.
        // It means that the book should match.
        $passage = $this->encodePassage ($book, NULL, NULL);
        $query .= " AND passage LIKE '%$passage%' ";
        break;
      case 3:
        // Select notes that refer to any passage: No constraint to apply here.
        break;
    }
    // Consider edit selector.
    switch ($edit_selector) {
      case 0:
        // Select notes that have been edited at any time. Apply no constraint.
        $time = 0;
        break;
      case 1:
        // Select notes that have been edited during the last 30 days.
        $time = strtotime ("today -30 days");
        break;
      case 2:
        // Select notes that have been edited during the last 7 days.
        $time = strtotime ("today -7 days");
        break;
      case 3:
        // Select notes that have been edited since yesterday.
        $time = strtotime ("yesterday");
        break;
      case 4:
        // Select notes that have been edited today.
        $time = strtotime ("today");
        break;
    }
    if ($time != 0) $query .= " AND modified >= $time ";
    // Consider non-edit selector.
    switch ($non_edit_selector) {
      case 0:
        // Select notes that have not been edited at any time. Apply no constraint.
        $nonedit = 0;
        break;
      case 1:
        // Select notes that have not been edited for a day.
        $nonedit = strtotime ("-1 day");
        break;
      case 2:
        // Select notes that have not been edited for two days.
        $nonedit = strtotime ("-2 days");
        break;
      case 3:
        // Select notes that have not been edited for a week.
        $nonedit = strtotime ("-1 week");
        break;
      case 4:
        // Select notes that have not been edited today.
        $nonedit = strtotime ("-1 month");
        break;
      case 5:
        // Select notes that have not been edited today.
        $nonedit = strtotime ("-1 year");
        break;
    }
    if ($nonedit != 0) $query .= " AND modified <= $nonedit ";
    // Consider status constraint.
    if ($status_selector != "") {
      $query .= " AND status = '$status_selector' ";
    }
    // Consider Bible constraints. 
    if ($bible_selector != "") {
      // A note can be a general one, not tied to any specific Bible.
      // Such notes should be selected as well, despite the $bible_selector's constraints.
      $query .= " AND (bible = '' OR bible = '$bible_selector') ";
    }
    // Consider note assignment constraints.
    if ($assignment_selector != "") {
      $assignment_selector = Database_SQLInjection::no ($assignment_selector);
      $query .= " AND assigned LIKE '% $assignment_selector %' ";
    }
    // Consider note subscription constraints.
    if ($subscription_selector == 1) {
      $query .= " AND subscriptions LIKE '% $username %' ";
    }
    // Consider the note severity.
    if ($severity_selector != -1) {
      $query .= " AND severity = $severity_selector ";
    }
    // Consider text contained in notes.
    if ($text_selector == 1) {
      $query .= Filter_Sql::notesOptionalFulltextSearchStatement ($search_text);
    }
    if ($text_selector == 1) {
      // If searching in fulltext mode, notes get ordered on relevance of search hits.
      $query .= Filter_Sql::notesOrderByRelevanceStatement ();
    } else {
      // Notes get ordered by the passage they refer to. It is a rough method and better ordering is needed. 
      $query .= " ORDER BY ABS (passage) ";
    }
    // Limit the selection if a limit is given.
    if (is_numeric ($limit)) {
      $limit = Database_SQLInjection::no ($limit);
      $query .= " LIMIT $limit, 50 ";
    }
    $query .= ";";
    $result = $server->runQuery ($query);
    for ($i = 0; $i < $result->num_rows; $i++) {
      $row = $result->fetch_row();
      $identifier = $row[0];
      $identifiers []= $identifier;
    }
    return $identifiers;
  }
  
  
  public function getSummary ($identifier)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $query = "SELECT summary FROM notes WHERE identifier = $identifier;";
    $result = $server->runQuery ($query);
    if ($result->num_rows > 0) {
      $row = $result->fetch_row();
      return $row[0];
    }
    return "";
  }
  

  public function setSummary ($identifier, $summary)
  {
    $server = Database_Instance::getInstance ();
    $summary = Database_SQLInjection::no ($summary);
    $query = "UPDATE notes SET summary = '$summary' WHERE identifier = $identifier;";
    $server->runQuery ($query);
    $this->updateSearchFields ($identifier);
  }


  public function getContents ($identifier)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $query = "SELECT contents FROM notes WHERE identifier = $identifier;";
    $result = $server->runQuery ($query);
    if ($result->num_rows > 0) {
      $row = $result->fetch_row();
      return $row[0];
    }
    return "";
  }
  
  
  public function setContents ($identifier, $contents)
  {
    $server = Database_Instance::getInstance ();
    $contents = Database_SQLInjection::no ($contents);
    $query = "UPDATE notes SET contents = '$contents' WHERE identifier = $identifier;";
    $server->runQuery ($query);
    $this->updateSearchFields ($identifier);
  }
  
  
  
  public function delete ($identifier)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $query = "DELETE FROM notes WHERE identifier = $identifier;";
    $result = $server->runQuery ($query);
  }

  /**
  * Add a $comment to an exiting note identified by $identifier.
  */
  public function addComment ($identifier, $comment)
  {
    $identifier = Database_SQLInjection::no ($identifier);
    if ($comment == "") return;
    $server = Database_Instance::getInstance ();
    $session_logic = Session_Logic::getInstance();
    $contents = $this->assembleContents ($identifier, $comment);
    $contents = Database_SQLInjection::no ($contents);
    $query = "UPDATE notes SET contents = '$contents' WHERE identifier = $identifier;";
    $server->runQuery ($query);
    $this->updateSearchFields ($identifier);
    $this->noteEditedActions ($identifier);
  }
  
  
  /**
  * Subscribe the current user to the note identified by $identifier.
  */
  public function subscribe ($identifier)
  {
    $session_logic = Session_Logic::getInstance();
    $user = $session_logic->currentUser ();
    $this->subscribeUser ($identifier, $user);
  }


  /**
  * Subscribe the $user to the note identified by $identifier.
  */
  public function subscribeUser ($identifier, $user)
  {
    // If the user already is subscribed to the note, bail out.
    $subscribers = $this->getSubscribers ($identifier);
    if (in_array ($user, $subscribers)) return;
    // Subscribe $user.
    $subscribers [] = $user;
    $this->setSubscribers ($identifier, $subscribers);
  }


  /**
  * Returns an array with the subscribers to the note identified by $identifier.
  */  
  public function getSubscribers ($identifier)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $query = "SELECT subscriptions FROM notes WHERE identifier = $identifier;";
    $result = $server->runQuery ($query);
    $row = $result->fetch_row();
    $subscribers = explode ("\n", $row[0]);
    $subscribers = array_diff ($subscribers, array (""));
    foreach ($subscribers as &$subscriber) {
      $subscriber = trim ($subscriber);
    }
    return $subscribers;
  }


  public function setSubscribers ($identifier, $subscribers)
  {
    // Add a space at both sides of the subscriber to allow for easier note selection based on note assignment.
    foreach ($subscribers as &$subscriber) {
      $subscriber = " $subscriber ";
   }
    $subscribers = implode ("\n", $subscribers);
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $subscribers = Database_SQLInjection::no ($subscribers);
    $query = "UPDATE notes SET subscriptions = '$subscribers' WHERE identifier = $identifier;";
    $server->runQuery ($query);
  }



  /**
  * Returns true if $user is subscribed to the note identified by $identifier.
  */
  public function isSubscribed ($identifier, $user)
  {
    $subscribers = $this->getSubscribers ($identifier);
    return in_array ($user, $subscribers);
  }

  
  /**
  * Unsubscribes the currently logged in user from the note identified by $identifier.
  */
  public function unsubscribe ($identifier)
  {
    $session_logic = Session_Logic::getInstance();
    $user = $session_logic->currentUser ();
    $this->unsubscribeUser ($identifier, $user);
  }


  /**
  * Unsubscribes $user from the note identified by $identifier.
  */
  public function unsubscribeUser ($identifier, $user)
  {
    // If the user is not subscribed to the note, bail out.
    $subscribers = $this->getSubscribers ($identifier);
    if (!in_array ($user, $subscribers)) return;
    // Unsubscribe $user.
    $subscribers = array_diff ($subscribers, array ($user));
    $this->setSubscribers ($identifier, $subscribers);
  }


  /**
  * Returns an array with all assignees to the notes.
  * These are the usernames to which one or more notes have been assigned.
  * This means that if no notes have been assigned to anybody, it will return an empty array.
  */
  public function getAllAssignees ()
  {
    $assignees = array ();
    $server = Database_Instance::getInstance ();
    $query = "SELECT DISTINCT assigned FROM notes;";
    $result = $server->runQuery ($query);
    for ($i = 0; $i < $result->num_rows; $i++) {
      $row = $result->fetch_row();
      $names = explode ("\n", $row[0]);
      $assignees = array_merge ($assignees, $names);
    }
    $assignees = array_unique ($assignees);
    foreach ($assignees as &$assignee) {
      $assignee = trim ($assignee);
    }
    $assignees = array_diff ($assignees, array (""));
    $assignees = array_values ($assignees);
    return $assignees;
  }
  

  /**
  * Returns an array with the assignees to the note identified by $identifier.
  */  
  public function getAssignees ($identifier)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $query = "SELECT assigned FROM notes WHERE identifier = $identifier;";
    $result = $server->runQuery ($query);
    $row = $result->fetch_row();
    $assignees = explode ("\n", $row[0]);
    $assignees = array_diff ($assignees, array (""));
    // Remove the padding space at both sides of the assignee.
    foreach ($assignees as &$assignee) {
      $assignee = trim ($assignee);
    }
    return $assignees;
  }
  
  /**
  * Sets the note's assignees.
  * $identifier : note identifier.
  * $assignees: array of user names.
  */
  public function setAssignees ($identifier, $assignees)
  {
    // Add a space at both sides of the assignee to allow for easier note selection based on note assignment.
    foreach ($assignees as &$assignee) {
      $assignee = " $assignee ";
   }
    $assignees = implode ("\n", $assignees);
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $assignees = Database_SQLInjection::no ($assignees);
    $query = "UPDATE notes SET assigned = '$assignees' WHERE identifier = $identifier;";
    $server->runQuery ($query);
    $this->noteEditedActions ($identifier);
  }


  /**
  * Assign the note identified by $identifier to $user.
  * $comment: Whether to add a comment to the notes upon assignment.
  */
  public function assignUser ($identifier, $user, $comment = true)
  {
    // If the note already is assigned to the user, bail out.
    $assignees = $this->getAssignees ($identifier);
    if (in_array ($user, $assignees)) return;
    // Assign the note to the user. 
    $assignees[]= "$user";
    // Store the whole log.
    $this->setAssignees ($identifier, $assignees);
    if ($comment) {
      // $this->addComment ($identifier, gettext ("The note was assigned to") . " " . $user);
    }
  }


  /**
  * Returns true if the note identified by $identifier has been assigned to $user.
  */
  public function isAssigned ($identifier, $user)
  {
    $assignees = $this->getAssignees ($identifier);
    return in_array ($user, $assignees);
  }

  
  /**
  * Unassignes the currently logged in user from the note identified by $identifier.
  */
  public function unassign ($identifier)
  {
    $session_logic = Session_Logic::getInstance();
    $user = $session_logic->currentUser ();
    $this->unassignUser ($identifier, $user);
  }


  /**
  * Unassigns $user from the note identified by $identifier.
  */
  public function unassignUser ($identifier, $user)
  {
    // If the notes is not assigned to the user, bail out.
    $assignees = $this->getAssignees ($identifier);
    if (!in_array ($user, $assignees)) return;
    // Remove assigned $user.
    $assignees = array_diff ($assignees, array ($user));
    $this->setAssignees ($identifier, $assignees);
    // $this->addComment ($identifier, gettext ("The note is no longer assigned to") . " " . $user);
  }

  

  public function getBible ($identifier)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $query = "SELECT bible FROM notes WHERE identifier = $identifier;";
    $result = $server->runQuery ($query);
    if ($result->num_rows > 0) {
      $row = $result->fetch_row();
      return $row[0];
    }
    return "";
  }


  public function setBible ($identifier, $bible)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $bible = Database_SQLInjection::no ($bible);
    $query = "UPDATE notes SET bible = '$bible' WHERE identifier = $identifier;";
    $server->runQuery ($query);
    $this->noteEditedActions ($identifier);
    // Add a comment for the change of Bible.
    // if ($bible == "") $bible = gettext ("none");
    // $this->addComment ($identifier, gettext ("The note's Bible was changed to:") . " " . $bible);
  }


  /**
  * Returns an array with all Bibles in the notes.
  */
  public function getAllBibles ()
  {
    $bibles = array ();
    $server = Database_Instance::getInstance ();
    $query = "SELECT DISTINCT bible FROM notes;";
    $result = $server->runQuery ($query);
    for ($i = 0; $i < $result->num_rows; $i++) {
      $row = $result->fetch_row();
      $bible = $row[0];
      if ($bible != "") {
        $bibles [] = $bible;
      }
    }
    return $bibles;
  }


  /**
  * Encodes the book, chapter and verse, like to, e.g.: "40.5.13",
  * and returns this as a string.
  */  
  public function encodePassage ($book, $chapter, $verse)
  {
    // Space before and after the passage enables notes selection on passage.
    // Special way of encoding, as done below, is to enable note selection on book / chapter / verse.
    $passage = " $book.";
    if ($chapter != "") $passage .= "$chapter.";
    if ($verse != "") $passage .= "$verse ";
    return $passage;
  }
  
  
  /**
  * Takes the $passage as a string, and returns an array with book, chapter, and verse.
  */
  public function decodePassage ($passage)
  {
    $passage = trim ($passage);
    return explode (".", $passage);
  }


  /**
  * Returns an array with the passages that the note identified by $identifier refers to.
  * Each passages is an array (book, chapter, verse).
  */  
  public function getPassages ($identifier)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $query = "SELECT passage FROM notes WHERE identifier = $identifier;";
    $result = $server->runQuery ($query);
    $row = $result->fetch_row();
    $lines = explode ("\n", $row[0]);
    $lines = array_diff ($lines, array (""));
    $passages = array ();
    foreach ($lines as $line) {
      $passage = $this->decodePassage ($line);
      $passages[] = $passage;
    }
    return $passages;
  }


  /**
  * Assign the passages to the note $identifier.
  * $passages is an array of an array (book, chapter, verse) passages.
  * $import: If true, just write passages, no further actions.
  */
  public function setPassages ($identifier, $passages, $import = false)
  {
    $server = Database_Instance::getInstance ();
    $line = "";
    foreach ($passages as $passage) {
      $line .= $this->encodePassage ($passage[0], $passage[1], $passage[2]);
      $line .= "\n";
    }
    $identifier = Database_SQLInjection::no ($identifier);
    $line = Database_SQLInjection::no ($line);
    $query = "UPDATE notes SET passage = '$line' WHERE identifier = $identifier;";
    $server->runQuery ($query);
    if (!$import) {
      $this->noteEditedActions ($identifier);
      // $this->addComment ($identifier, gettext ("The note's passages were updated"));
    }
  }


  /**
  * Add the passage of $book, $chapter, $verse to the note identified by $identifier.
  */
  public function addPassage ($identifier, $book, $chapter, $verse)
  {
    // If the passage is already in, bail out.
    $passage = array ($book, $chapter, $verse);
    $passages = $this->getPassages ($identifier);
    if (in_array ($passage, $passages)) return;
    // Add the passage to the note.
    $passages[]= $passage;
    $this->setPassages ($identifier, $passages);
  }


  /**
  * Removes $passage from the note identified by $identifier.
  */
  public function removePassage ($identifier, $book, $chapter, $verse)
  {
    // A special method is used to remove the passage, because array_diff 
    // does not operate properly on multi-dimensional arrays.
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $query = "SELECT passage FROM notes WHERE identifier = $identifier;";
    $result = $server->runQuery ($query);
    $row = $result->fetch_row();
    $lines = explode ("\n", $row[0]);
    $lines = array_diff ($lines, array (""));
    $lines = array_diff ($lines, array ($this->encodePassage ($book, $chapter, $verse)));
    $passages = array ();
    foreach ($lines as $line) {
      $passage = $this->decodePassage ($line);
      $passages[] = $passage;
    }
    $this->setPassages ($identifier, $passages);
  }


  /**
  * Returns true if the $passage is contained in the note identified by $identifier.
  */
  public function passageContained ($identifier, $book, $chapter, $verse)
  {
    $passages = $this->getPassages ($identifier);
    $passage = array ($book, $chapter, $verse);
    return in_array ($passage, $passages);
  }



  /**
  * Returns the raw passage text of the note identified by $identifier.
  */  
  public function getRawPassage ($identifier)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $query = "SELECT passage FROM notes WHERE identifier = $identifier;";
    $result = $server->runQuery ($query);
    $row = $result->fetch_row();
    $rawpassage = $row[0];
    return $rawpassage;
  }


  /**
  * Gets the raw status of a note.
  * Returns it as a string.
  */
  public function getRawStatus ($identifier)
  {
    // Get status.
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $query = "SELECT status FROM notes WHERE identifier = $identifier;";
    $result = $server->runQuery ($query);
    $status = "";
    if ($result->num_rows > 0) {
      $row = $result->fetch_row();
      $status = $row[0];
    }
    return $status;
  }


  /**
  * Gets the localized status of a note.
  * Returns it as a string.
  */
  public function getStatus ($identifier)
  {
     $status = $this->getRawStatus ($identifier);
    // Localize status if it is a standard one.
    if (in_array ($status, $this->standard_statuses)) $status = gettext ($status);
    // Return status.
    return $status;
  }


  /**
  * Sets the $status of the note identified by $identifier.
  * $status is a string.
  * $import: Just write status, skip any logic.
  */
  public function setStatus ($identifier, $status, $import = false)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $status = Database_SQLInjection::no ($status);
    $query = "UPDATE notes SET status = '$status' WHERE identifier = $identifier;";
    $server->runQuery ($query);
    if (!$import) {
      $this->noteEditedActions ($identifier);
      // $this->addComment ($identifier, gettext ("The note's status was updated"));
    }
  }


  /**
  * Gets an array of array with the possible statuses of consultation notes, 
  * both raw and localized versions.
  */
  public function getPossibleStatuses ()
  {
    // Get an array with the statuses used in the database, ordered by occurrence, most often used ones first.
    $statuses = array ();
    $database_instance = Database_Instance::getInstance();
    $query = "SELECT status, COUNT(status) AS occurrences FROM notes GROUP BY status ORDER BY occurrences DESC;";
    $result = $database_instance->runQuery ($query);
    for ($i = 0; $i <$result->num_rows ; $i++) {
      $row = $result->fetch_row ();
      $statuses[] = $row[0];
    }
    // Ensure the standard statuses are there too.
    foreach ($this->standard_statuses as $standard_status) {
      if (!in_array ($standard_status, $statuses)) {
        $statuses[] = $standard_status;
      }
    }
    // Localize the results.
    foreach ($statuses as $status) {
      $localization = $status;
      if (in_array ($status, $this->standard_statuses)) $localization = gettext ($status);
      $localized_status = array ($status, $localization);
      $localized_statuses [] = $localized_status;
    }
    // Return result.
    return $localized_statuses;
  }
  

  /**
  * Returns the severity of a note as a number.
  */
  public function getRawSeverity ($identifier)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $query = "SELECT severity FROM notes WHERE identifier = $identifier;";
    $result = $server->runQuery ($query);
    $severity = 2;
    if ($result->num_rows > 0) {
      $row = $result->fetch_row();
      $severity = $row[0];
    }
    return $severity;
  }


  /**
  * Returns the severity of a note as a localized string.
  */
  public function getSeverity ($identifier)
  {
    $severity = $this->getRawSeverity ($identifier);
    $severity = $this->standard_severities[$severity];
    if ($severity == "") $severity = "Normal";
    $severity = gettext ($severity);
    return $severity;
  }


  /**
  * Sets the $severity of the note identified by $identifier.
  * $severity is a number.
  */
  public function setRawSeverity ($identifier, $severity)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $severity = Database_SQLInjection::no ($severity);
    $query = "UPDATE notes SET severity = $severity WHERE identifier = $identifier;";
    $server->runQuery ($query);
    $this->noteEditedActions ($identifier);
    // $this->addComment ($identifier, gettext ("The note's severity was updated"));
  }


  /**
  * Gets an array with the possible severities.
  */
  public function getPossibleSeverities ()
  {
    for ($i = 0; $i <= 5; $i++) {
      $severities[] = array ($i, gettext ($this->standard_severities[$i]));
    }
    return $severities;
  }
  

  /**
  * Returns the privacy of a note as a number.
  */
  public function getPrivacy ($identifier)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $query = "SELECT private FROM notes WHERE identifier = $identifier;";
    $result = $server->runQuery ($query);
    $privacy = 0;
    if ($result->num_rows > 0) {
      $row = $result->fetch_row();
      $privacy = $row[0];
    }
    return $privacy;
  }


  /**
  * Sets the $privacy of the note identified by $identifier.
  * $privacy is a number.
  */
  public function setPrivacy ($identifier, $privacy)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $privacy = Database_SQLInjection::no ($privacy);
    $query = "UPDATE notes SET private = $privacy WHERE identifier = $identifier;";
    $server->runQuery ($query);
    $this->noteEditedActions ($identifier);
    // $this->addComment ($identifier, gettext ("The note's privacy was updated"));
  }


  /**
  * Gets an array with the possible privacy values.
  */
  public function getPossiblePrivacies ()
  {
    include ("session/levels.php");
    for ($i = GUEST_LEVEL; $i <= ADMIN_LEVEL; $i++) {
      $privacies[] = $i;
    }
    return $privacies;
  }

  
  public function getModified ($identifier)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $query = "SELECT modified FROM notes WHERE identifier = $identifier;";
    $result = $server->runQuery ($query);
    if ($result->num_rows > 0) {
      $row = $result->fetch_row();
      return $row[0];
    }
    return 0;
  }


  public function setModified ($identifier, $time)
  {
    $server = Database_Instance::getInstance ();
    $identifier = Database_SQLInjection::no ($identifier);
    $time = Database_SQLInjection::no ($time);
    $query = "UPDATE notes SET modified = $time WHERE identifier = $identifier;";
    $server->runQuery ($query);
  }
  

  /**
  * Takes actions when a note has been edited.
  * $identifier - the note.
  */
  private function noteEditedActions ($identifier)
  {
    $server = Database_Instance::getInstance ();
    // Update 'modified' field.
    $modified = time();
    $query = "UPDATE notes SET modified = $modified WHERE identifier = $identifier;";
    $server->runQuery ($query);
  }


  /**
  * Returns an array of duplicate note identifiers selected.
  */
  public function selectDuplicateNotes ($rawpassage, $summary, $contents)
  {
    $identifiers = array ();
    $server = Database_Instance::getInstance ();
    $rawpassage = Database_SQLInjection::no ($rawpassage);
    $summary = Database_SQLInjection::no ($summary);
    $contents = Database_SQLInjection::no ($contents);
    $query = "SELECT identifier FROM notes WHERE passage = '$rawpassage' AND summary = '$summary' AND contents = '$contents';";
    $result = $server->runQuery ($query);
    for ($i = 0; $i < $result->num_rows; $i++) {
      $row = $result->fetch_row();
      $identifier = $row[0];
      $identifiers []= $identifier;
    }
    return $identifiers;
  }


  public function updateSearchFields ($identifier)
  {
    // The search field is a combination of the summary and content converted to clean text.
    // Another search field will contain the reversed clean text.
    // Both fields together enable us to search with wildcards before and after the search query
    // with MySQL fulltext search in boolean mode.
    $noteSummary = $this->getSummary ($identifier);
    $noteContents = $this->getContents ($identifier);
    $cleanText = $noteSummary . "\n" . Filter_Html::html2text ($noteContents);
    $reversedText = Filter_String::reverse ($cleanText);
    $cleanText = Database_SQLInjection::no ($cleanText);
    $reversedText = Database_SQLInjection::no ($reversedText);
    $server = Database_Instance::getInstance ();
    $query = "UPDATE notes SET cleantext = '$cleanText' WHERE identifier = $identifier;";
    $server->runQuery ($query);
    $query = "UPDATE notes SET reversedtext = '$reversedText' WHERE identifier = $identifier;";
    $server->runQuery ($query);
  }


  /**
  * Searches the notes.
  * Returns an array of note identifiers.
  * If $limit is non-NULL, it indicates the starting limit for the selection.
  * $search: Contains the text to search for.
  */
  public function searchNotes ($search, $limit = NULL)
  {
    $identifiers = array ();
    $server = Database_Instance::getInstance ();
    // SQL SELECT statement.
    $query = Filter_Sql::notesSelectIdentifier ();
    // SQL fulltext search statement sorted on relevance.
    $query .= Filter_Sql::notesOptionalFulltextSearchRelevanceStatement ($search);
    // SQL FROM ... WHERE statement.
    $query .= Filter_Sql::notesFromWhereStatement ();
    // SQL privacy statement.
    $session_logic = Session_Logic::getInstance ();
    $userlevel = $session_logic->currentLevel ();
    $query .= Filter_Sql::notesConsiderPrivacy ($userlevel);
    // Consider text contained in notes.
    $query .= Filter_Sql::notesOptionalFulltextSearchStatement ($search);
    // Notes get ordered on relevance of search hits.
    $query .= Filter_Sql::notesOrderByRelevanceStatement ();
    // Limit the selection if a limit is given.
    if (is_numeric ($limit)) {
      $limit = Database_SQLInjection::no ($limit);
      $query .= " LIMIT $limit, 50 ";
    }
    $query .= ";";
    $result = $server->runQuery ($query);
    for ($i = 0; $i < $result->num_rows; $i++) {
      $row = $result->fetch_row();
      $identifier = $row[0];
      $identifiers []= $identifier;
    }
    return $identifiers;
  }
  
  
}


?>
