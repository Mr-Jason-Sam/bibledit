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


require_once ("../bootstrap/bootstrap.php");


// The script runs through the cli Server API only.
if (php_sapi_name () != "cli") {
  die;
}


// Change the user id for running this script.
$pwnam = posix_getpwnam ("www-data");
posix_setuid ($pwnam['uid']);
posix_setgid ($pwnam['gid']);
posix_seteuid ($pwnam['uid']);
posix_setegid ($pwnam['gid']);


ignore_user_abort (true);
set_time_limit (0);


$config_general = Database_Config_General::getInstance ();
$timer_logger = new Timer_Logger ();
//$processUser = posix_getpwuid(posix_geteuid());
//$processUser = $processUser['name'];


$current_timestamp = time ();


// CPU-intensive actions run at night.
// This keeps the site more responsive during the day.
$hour = date ('G');
$minute = date ('i');


// Every minute send out any queued mail.
$workingdirectory = escapeshellarg (dirname (__FILE__));
$logfilename = $timer_logger->getLogFilename (Timer_Logger::mailer);
$command = "cd $workingdirectory; php mailer.php > $logfilename 2>&1 & echo $!";
$pid = shell_exec ($command);
$timer_logger->registerLogfile ($command, $pid, $logfilename, false);


// Every minute deal with any log files that were used for the scripts.
$timer_logger->handleUsedLogFiles ();


// Check for new mail every five minutes.
// Do not check more often with gmail else the account may be shut down.
if (($minute % 5) == 0) {
  $workingdirectory = escapeshellarg (dirname (__FILE__));
  $logfilename = $timer_logger->getLogFilename (Timer_Logger::receiver);
  $command = "cd $workingdirectory; php receiver.php > $logfilename 2>&1 & echo $!";
  $pid = shell_exec ($command);
  $timer_logger->registerLogfile ($command, $pid, $logfilename, false);
}


// The running order of the following nightly scripts is important.
// In general, the order will such that all information is generated as recent as possible,
// and that the more important tasks are done first, and the less important ones at the end.
// This means something as displayed in the code of this script.


// Sending and receiving Bibles to and from the git repository.
// On a production website running on an inexpensive virtual private server 
// with 512 Mbyte of memory and a fast network connection, 
// sending and receiving two Bibles takes more than five minutes when there are many changes.
if (($current_timestamp >= $config_general->getTimerSendReceive ()) || (($hour == 0) && ($minute == 0))) {
  $config_general->setTimerSendReceive ($current_timestamp + 100000);
  $workingdirectory = escapeshellarg (dirname (__FILE__));
  $logfilename = $timer_logger->getLogFilename (Timer_Logger::sendreceive);
  $command = "cd $workingdirectory; php sendreceive.php > $logfilename 2>&1 & echo $!";
  $pid = shell_exec ($command);
  $timer_logger->registerLogfile ($command, $pid, $logfilename);
}


// Sending the daily changes in the Bibles by email.
// This takes a few minutes on a production machine with two Bibles and changes in several chapters.
if (($current_timestamp >= $config_general->getTimerDiff ()) || (($hour == 0) && ($minute == 10))) {
  $config_general->setTimerDiff ($current_timestamp + 100000);
  $workingdirectory = escapeshellarg (dirname (__FILE__));
  $logfilename = $timer_logger->getLogFilename (Timer_Logger::changes);
  $command = "cd $workingdirectory; php changes.php > $logfilename 2>&1 & echo $!";
  $pid = shell_exec ($command);
  $timer_logger->registerLogfile ($command, $pid, $logfilename);
}


// Run the checks on the Bibles.
// This takes 15 minutes on a production machine with two Bibles.
if (($current_timestamp >= $config_general->getTimerChecks ()) || (($hour == 0) && ($minute == 15))) {
  $config_general->setTimerChecks ($current_timestamp + 100000);
  $workingdirectory = dirname (__FILE__);
  $logfilename = $timer_logger->getLogFilename (Timer_Logger::checks);
  $command = "cd $workingdirectory; php checks.php > $logfilename 2>&1 & echo $!";
  $pid = shell_exec ($command);
  $timer_logger->registerLogfile ($command, $pid, $logfilename);
}


// Database maintenance and trimming.
// It takes a few minutes on a production machine..
if (($hour == 0) && ($minute == 35)) {
  $workingdirectory = dirname (__FILE__);
  $logfilename = $timer_logger->getLogFilename (Timer_Logger::database);
  $command = "cd $workingdirectory; php database.php > $logfilename 2>&1 & echo $!";
  $pid = shell_exec ($command);
  $timer_logger->registerLogfile ($command, $pid, $logfilename);
}


// Create a backup, so that the backup contains the most recent information
// after the previous tasks have been done.
if (($current_timestamp >= $config_general->getTimerBackup ()) || (($hour == 0) && ($minute == 45))) {
  $config_general->setTimerBackup ($current_timestamp + 100000);
  $workingdirectory = dirname (__FILE__);
  $logfilename = $timer_logger->getLogFilename (Timer_Logger::backup);
  $command = "cd $workingdirectory; php backup.php > $logfilename 2>&1 & echo $!";
  $pid = shell_exec ($command);
  $timer_logger->registerLogfile ($command, $pid, $logfilename);
}


// Email statistics to the users.
if (($hour == 0) && ($minute == 50)) {
  $workingdirectory = escapeshellarg (dirname (__FILE__));
  $logfilename = $timer_logger->getLogFilename (Timer_Logger::statistics);
  $command = "cd $workingdirectory; php statistics.php > $logfilename 2>&1 & echo $!";
  $pid = shell_exec ($command);
  $timer_logger->registerLogfile ($command, $pid, $logfilename);
}


// Export the Bibles to the various output formats.
// This may take an hour on a production machine.
if (($current_timestamp >= $config_general->getTimerExports ()) || (($hour == 0) && ($minute == 55))) {
  $config_general->setTimerExports ($current_timestamp + 100000);
  $workingdirectory = escapeshellarg (dirname (__FILE__));
  $logfilename = $timer_logger->getLogFilename (Timer_Logger::exports);
  $command = "cd $workingdirectory; php exports.php > $logfilename 2>&1 & echo $!";
  $pid = shell_exec ($command);
  $timer_logger->registerLogfile ($command, $pid, $logfilename);
}


?>
