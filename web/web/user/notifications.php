<?php
require_once ("../bootstrap/bootstrap.php");
page_access_level (MEMBER_LEVEL);
Assets_Page::header (gettext ("Notifications"));
$view = new Assets_View (__FILE__);

$database_config_user = Database_Config_User::getInstance ();

if (isset ($_GET['editednotessubscriptiontoggle'])) {
  $database_config_user->setSubscribeToConsultationNotesEditedByMe(!$database_config_user->getSubscribeToConsultationNotesEditedByMe());
}
$view->view->edited_notes_subscription = $database_config_user->getSubscribeToConsultationNotesEditedByMe();

if (isset ($_GET['anynotessubscriptiontoggle'])) {
  $database_config_user->setNotifyMeOfAnyConsultationNotesEdits(!$database_config_user->getNotifyMeOfAnyConsultationNotesEdits());
}
$view->view->any_notes_subscription = $database_config_user->getNotifyMeOfAnyConsultationNotesEdits();

if (isset ($_GET['emailconfirmationyourpoststoggle'])) {
  $database_config_user->setNotifyMeOfMyPosts (!$database_config_user->getNotifyMeOfMyPosts ());
}
$view->view->email_confirmation_your_posts = $database_config_user->getNotifyMeOfMyPosts ();

if (isset ($_GET['subscribednotenotificationtoggle'])) {
  $database_config_user->setSubscribedConsultationNoteNotification(!$database_config_user->getSubscribedConsultationNoteNotification());
}
$view->view->subscription_notification = $database_config_user->getSubscribedConsultationNoteNotification();

if (isset ($_GET['notesassignmenttoggle'])) {
  $database_config_user->setAssignedToConsultationNotesChanges(!$database_config_user->getAssignedToConsultationNotesChanges());
}
$view->view->notes_assignment = $database_config_user->getAssignedToConsultationNotesChanges();

if (isset ($_GET['assignednotenotificationtoggle'])) {
  $database_config_user->setAssignedConsultationNoteNotification(!$database_config_user->getAssignedConsultationNoteNotification());
}
$view->view->assignment_notification = $database_config_user->getAssignedConsultationNoteNotification();

if (isset ($_GET['suppressemailsfromnotesyouupdatedtoggle'])) {
  $database_config_user->setSuppressMailFromYourUpdatesNotes (!$database_config_user->getSuppressMailFromYourUpdatesNotes ());
}
$view->view->suppress_mails_from_your_updates = $database_config_user->getSuppressMailFromYourUpdatesNotes ();

if (isset ($_GET['assignednotesnotificationtoggle'])) {
  $database_config_user->setAssignedNotesStatisticsNotification (!$database_config_user->getAssignedNotesStatisticsNotification ());
}
$view->view->assigned_notes_notifications = $database_config_user->getAssignedNotesStatisticsNotification ();

if (isset ($_GET['deletednotenotificationtoggle'])) {
  $database_config_user->setDeletedConsultationNoteNotification(!$database_config_user->getDeletedConsultationNoteNotification());
}
$view->view->deleted_note_notification = $database_config_user->getDeletedConsultationNoteNotification();

if (isset ($_GET['biblechangesnotificationtoggle'])) {
  $database_config_user->setBibleChangesNotification(!$database_config_user->getBibleChangesNotification());
}
$view->view->bible_changes_notification = $database_config_user->getBibleChangesNotification();

if (isset ($_GET['changenotificationstoggle'])) {
  $database_config_user->setGenerateChangeNotifications(!$database_config_user->getGenerateChangeNotifications());
}
$view->view->change_notifications = $database_config_user->getGenerateChangeNotifications();

if (isset ($_GET['pendingchangenotificationstoggle'])) {
  $database_config_user->setPendingChangesNotification (!$database_config_user->getPendingChangesNotification ());
}
$view->view->pending_changes_notifications = $database_config_user->getPendingChangesNotification ();

if (isset ($_GET['biblechecksnotificationtoggle'])) {
  $database_config_user->setBibleChecksNotification (!$database_config_user->getBibleChecksNotification ());
}
$view->view->bible_checks_notification = $database_config_user->getBibleChecksNotification ();

$view->render ("notifications.php");
Assets_Page::footer ();
?>
