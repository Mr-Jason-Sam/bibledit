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


#include <filter/url.h>
#include <index/index.h>
#include <bootstrap/bootstrap.h>
#include <webserver/http.h>
#include <session/login.h>
#include <session/logout.h>
#include <session/password.h>
#include <session/signup.h>
#include <database/config/general.h>
#include <database/offlineresources.h>
#include <setup/index.h>
#include <journal/index.h>
#include <config/logic.h>
#include <help/index.h>
#include <email/index.h>
#include <user/notifications.h>
#include <user/account.h>
#include <manage/indexing.h>
#include <manage/users.h>
#include <administration/language.h>
#include <administration/timezone.h>
#include <collaboration/index.h>
#include <collaboration/flash.h>
#include <collaboration/open.h>
#include <collaboration/password.h>
#include <collaboration/secure.h>
#include <collaboration/direction.h>
#include <styles/indext.h>
#include <styles/indexm.h>
#include <styles/sheetm.h>
#include <styles/view.h>
#include <fonts/index.h>
#include <versification/index.h>
#include <versification/system.h>
#include <bible/manage.h>
#include <bible/settings.h>
#include <bible/book.h>
#include <bible/chapter.h>
#include <bible/import_usfm.h>
#include <bible/import_bibleworks.h>
#include <bible/abbreviations.h>
#include <bible/order.h>
#include <bible/css.h>
#include <bible/editing.h>
#include <compare/index.h>
#include <jobs/index.h>
#include <editverse/index.h>
#include <editverse/id.h>
#include <editverse/load.h>
#include <editverse/save.h>
#include <navigation/update.h>
#include <navigation/poll.h>
#include <editusfm/index.h>
#include <editusfm/focus.h>
#include <editusfm/id.h>
#include <editusfm/load.h>
#include <editusfm/offset.h>
#include <editusfm/save.h>
#include <edit/index.h>
#include <edit/edit.h>
#include <edit/focus.h>
#include <edit/id.h>
#include <edit/load.h>
#include <edit/offset.h>
#include <edit/save.h>
#include <edit/styles.h>
#include <search/search.h>
#include <search/index.h>
#include <search/select.h>
#include <search/replace.h>
#include <search/getids.h>
#include <search/replacepre.h>
#include <search/replacego.h>
#include <search/search2.h>
#include <search/replace2.h>
#include <search/replacepre2.h>
#include <search/getids2.h>
#include <search/replacego2.h>
#include <search/similar.h>
#include <search/strongs.h>
#include <search/strong.h>
#include <search/originals.h>
#include <workbench/index.h>
#include <workbench/organize.h>
#include <workbench/settings.h>
#include <sendreceive/index.h>
#include <client/index.h>
#include <sync/setup.h>
#include <sync/settings.h>
#include <sync/bibles.h>
#include <sync/notes.h>
#include <sync/usfmresources.h>
#include <sync/externalresources.h>
#include <resource/index.h>
#include <resource/organize.h>
#include <resource/get.h>
#include <resource/bible2resource.h>
#include <resource/manage.h>
#include <resource/admin.h>
#include <resource/print.h>
#include <resource/download.h>
#include <mapping/index.h>
#include <mapping/map.h>
#include <notes/index.h>
#include <notes/poll.h>
#include <notes/notes.h>
#include <notes/create.h>
#include <notes/select.h>
#include <notes/note.h>
#include <notes/comment.h>
#include <notes/actions.h>
#include <notes/assign-1.h>
#include <notes/assign-n.h>
#include <notes/unassign-n.h>
#include <notes/status-1.h>
#include <notes/status-n.h>
#include <notes/verses.h>
#include <notes/severity-1.h>
#include <notes/severity-n.h>
#include <notes/bible-1.h>
#include <notes/bible-n.h>
#include <notes/bulk.h>
#include <notes/editsource.h>
#include <notes/summary.h>
#include <notes/click.h>
#include <changes/changes.h>
#include <changes/change.h>
#include <changes/manage.h>
#include <index/listing.h>
#include <sprint/index.h>
#include <checks/index.h>
#include <checks/settings.h>
#include <checks/settingspatterns.h>
#include <checks/settingssentences.h>
#include <consistency/index.h>
#include <checks/suppress.h>
#include <consistency/poll.h>
#include <consistency/input.h>
#include <manage/exports.h>
#include <manage/hyphenation.h>
#include <xrefs/index.h>
#include <xrefs/source.h>
#include <xrefs/target.h>
#include <xrefs/help.h>
#include <xrefs/extract.h>
#include <xrefs/interpret.h>
#include <xrefs/translate.h>
#include <xrefs/clear.h>
#include <xrefs/insert.h>
#include <xrefs/move.h>
#include <xrefs/next.h>


// This function is the first function to be called when a client requests a page or file.
// Based on the request from the client, it decides what other functions to call to assemble the response.
void bootstrap_index (Webserver_Request * request)
{
  string extension = filter_url_get_extension (request->get);
  string url = request->get.substr (1);
  
  // Serve graphics, stylesheets, JavaScript, fonts.
  if (   (extension == "ico")
      || (extension == "png")
      || (extension == "gif")
      || (extension == "css")
      || (extension == "js")
      || (extension == "ttf")
      || (extension == "otf")
      || (extension == "otf")
      || (extension == "woff")
      ) http_serve_file (request);
  
  // Serve offline resources.
  else if ((request->get.find (Database_OfflineResources::offlineresources ()) != string::npos) && (extension == "sqlite")) http_serve_file (request);
  
  // Force setup.
  else if (config_logic_version () != Database_Config_General::getInstalledInterfaceVersion ()) request->reply = setup_index (request);

  // Home page.
  else if ((url == index_index_url ()) && index_index_acl (request)) request->reply = index_index (request);
  
  // Login and logout.
  else if ((url == session_login_url ()) && session_login_acl (request)) request->reply = session_login (request);
  else if ((url == session_logout_url ()) && session_logout_acl (request)) request->reply = session_logout (request);
  else if ((url == session_password_url ()) && session_password_acl (request)) request->reply = session_password (request);
  else if ((url == session_signup_url ()) && session_signup_acl (request)) request->reply = session_signup (request);
  
  // Bible menu.
  else if ((url == bible_manage_url ()) && bible_manage_acl (request)) request->reply = bible_manage (request);
  else if ((url == bible_settings_url ()) && bible_settings_acl (request)) request->reply = bible_settings (request);
  else if ((url == bible_book_url ()) && bible_book_acl (request)) request->reply = bible_book (request);
  else if ((url == bible_chapter_url ()) && bible_chapter_acl (request)) request->reply = bible_chapter (request);
  else if ((url == bible_import_usfm_url ()) && bible_import_usfm_acl (request)) request->reply = bible_import_usfm (request);
  else if ((url == bible_import_bibleworks_url ()) && bible_import_bibleworks_acl (request)) request->reply = bible_import_bibleworks (request);
  else if ((url == compare_index_url ()) && compare_index_acl (request)) request->reply = compare_index (request);
  else if ((url == bible_abbreviations_url ()) && bible_abbreviations_acl (request)) request->reply = bible_abbreviations (request);
  else if ((url == bible_order_url ()) && bible_order_acl (request)) request->reply = bible_order (request);
  else if ((url == bible_css_url ()) && bible_css_acl (request)) request->reply = bible_css (request);
  else if ((url == editverse_index_url ()) && editverse_index_acl (request)) request->reply = editverse_index (request);
  else if ((url == editusfm_index_url ()) && editusfm_index_acl (request)) request->reply = editusfm_index (request);
  else if ((url == edit_index_url ()) && edit_index_acl (request)) request->reply = edit_index (request);
  else if ((url == search_index_url ()) && search_index_acl (request)) request->reply = search_index (request);
  else if ((url == workbench_index_url ()) && workbench_index_acl (request)) request->reply = workbench_index (request);
  else if ((url == workbench_organize_url ()) && workbench_organize_acl (request)) request->reply = workbench_organize (request);
  else if ((url == bible_editing_url ()) && bible_editing_acl (request)) request->reply = bible_editing (request);
  else if ((url == resource_bible2resource_url ()) && resource_bible2resource_acl (request)) request->reply = resource_bible2resource (request);
  else if ((url == checks_index_url ()) && checks_index_acl (request)) request->reply = checks_index (request);
  else if ((url == checks_settings_url ()) && checks_settings_acl (request)) request->reply = checks_settings (request);
  else if ((url == consistency_index_url ()) && consistency_index_acl (request)) request->reply = consistency_index (request);
  
  // Notes menu.
  else if ((url == notes_index_url ()) && notes_index_acl (request)) request->reply = notes_index (request);
  else if ((url == notes_create_url ()) && notes_create_acl (request)) request->reply = notes_create (request);
  else if ((url == notes_select_url ()) && notes_select_acl (request)) request->reply = notes_select (request);
  else if ((url == notes_note_url ()) && notes_note_acl (request)) request->reply = notes_note (request);
  else if ((url == notes_comment_url ()) && notes_comment_acl (request)) request->reply = notes_comment (request);
  else if ((url == notes_actions_url ()) && notes_actions_acl (request)) request->reply = notes_actions (request);
  else if ((url == notes_assign_1_url ()) && notes_assign_1_acl (request)) request->reply = notes_assign_1 (request);
  else if ((url == notes_assign_n_url ()) && notes_assign_n_acl (request)) request->reply = notes_assign_n (request);
  else if ((url == notes_unassign_n_url ()) && notes_unassign_n_acl (request)) request->reply = notes_unassign_n (request);
  else if ((url == notes_status_1_url ()) && notes_status_1_acl (request)) request->reply = notes_status_1 (request);
  else if ((url == notes_status_n_url ()) && notes_status_n_acl (request)) request->reply = notes_status_n (request);
  else if ((url == notes_verses_url ()) && notes_verses_acl (request)) request->reply = notes_verses (request);
  else if ((url == notes_severity_1_url ()) && notes_severity_1_acl (request)) request->reply = notes_severity_1 (request);
  else if ((url == notes_severity_n_url ()) && notes_severity_n_acl (request)) request->reply = notes_severity_n (request);
  else if ((url == notes_bible_1_url ()) && notes_bible_1_acl (request)) request->reply = notes_bible_1 (request);
  else if ((url == notes_bible_n_url ()) && notes_bible_n_acl (request)) request->reply = notes_bible_n (request);
  else if ((url == notes_bulk_url ()) && notes_bulk_acl (request)) request->reply = notes_bulk (request);
  else if ((url == notes_editsource_url ()) && notes_editsource_acl (request)) request->reply = notes_editsource (request);
  else if ((url == notes_summary_url ()) && notes_summary_acl (request)) request->reply = notes_summary (request);

  // Resources menu.
  else if ((url == resource_index_url ()) && resource_index_acl (request)) request->reply = resource_index (request);
  else if ((url == resource_organize_url ()) && resource_organize_acl (request)) request->reply = resource_organize (request);
  else if ((url == resource_print_url ()) && resource_print_acl (request)) request->reply = resource_print (request);
  else if ((url == resource_manage_url ()) && resource_manage_acl (request)) request->reply = resource_manage (request);
  else if ((url == resource_admin_url ()) && resource_admin_acl (request)) request->reply = resource_admin (request);
  else if ((url == resource_download_url ()) && resource_download_acl (request)) request->reply = resource_download (request);
  
  // Changes menu.
  else if ((url == journal_index_url ()) && journal_index_acl (request)) request->reply = journal_index (request);
  else if ((url == changes_changes_url ()) && changes_changes_acl (request)) request->reply = changes_changes (request);
  else if ((url == changes_change_url ()) && changes_change_acl (request)) request->reply = changes_change (request);
  else if ((url == changes_manage_url ()) && changes_manage_acl (request)) request->reply = changes_manage (request);

  // Planning menu.
  else if ((url == sprint_index_url ()) && sprint_index_acl (request)) request->reply = sprint_index (request);
  
  // Tools menu.
  else if ((url == sendreceive_index_url ()) && sendreceive_index_acl (request)) request->reply = sendreceive_index (request);
  else if ((url == manage_exports_url ()) && manage_exports_acl (request)) request->reply = manage_exports (request);
  else if ((url == manage_hyphenation_url ()) && manage_hyphenation_acl (request)) request->reply = manage_hyphenation (request);
  else if ((url == xrefs_index_url ()) && xrefs_index_acl (request)) request->reply = xrefs_index (request);
  
  // Settings menu.
  else if ((url == manage_users_url ()) && manage_users_acl (request)) request->reply = manage_users (request);
  else if ((url == manage_indexing_url ()) && manage_indexing_acl (request)) request->reply = manage_indexing (request);
  else if ((url == administration_language_url ()) && administration_language_acl (request)) request->reply = administration_language (request);
  else if ((url == administration_timezone_url ()) && administration_timezone_acl (request)) request->reply = administration_timezone (request);
  else if ((url == email_index_url ()) && email_index_acl (request)) request->reply = email_index (request);
  else if ((url == styles_indext_url ()) && styles_indext_acl (request)) request->reply = styles_indext (request);
  else if ((url == styles_indexm_url ()) && styles_indexm_acl (request)) request->reply = styles_indexm (request);
  else if ((url == styles_sheetm_url ()) && styles_sheetm_acl (request)) request->reply = styles_sheetm (request);
  else if ((url == styles_view_url ()) && styles_view_acl (request)) request->reply = styles_view (request);
  else if ((url == versification_index_url ()) && versification_index_acl (request)) request->reply = versification_index (request);
  else if ((url == versification_system_url ()) && versification_system_acl (request)) request->reply = versification_system (request);
  else if ((url == collaboration_index_url ()) && collaboration_index_acl (request)) request->reply = collaboration_index (request);
  else if ((url == client_index_url ()) && client_index_acl (request)) request->reply = client_index (request);
  else if ((url == fonts_index_url ()) && fonts_index_acl (request)) request->reply = fonts_index (request);
  else if ((url == mapping_index_url ()) && mapping_index_acl (request)) request->reply = mapping_index (request);
  else if ((url == mapping_map_url ()) && mapping_map_acl (request)) request->reply = mapping_map (request);
  
  // Help menu.
  else if ((help_index_url (url)) && help_index_acl (request, url)) request->reply = help_index (request, url);

  // User menu.  
  else if ((url == user_notifications_url ()) && user_notifications_acl (request)) request->reply = user_notifications (request);
  else if ((url == user_account_url ()) && user_account_acl (request)) request->reply = user_account (request);

  // Pages not in any menu.
  else if ((url == jobs_index_url ()) && jobs_index_acl (request)) request->reply = jobs_index (request);
  else if ((url == search_search_url ()) && search_search_acl (request)) request->reply = search_search (request);
  else if ((url == search_select_url ()) && search_select_acl (request)) request->reply = search_select (request);
  else if ((url == search_replace_url ()) && search_replace_acl (request)) request->reply = search_replace (request);
  else if ((url == search_search2_url ()) && search_search2_acl (request)) request->reply = search_search2 (request);
  else if ((url == search_replace2_url ()) && search_replace2_acl (request)) request->reply = search_replace2 (request);
  else if ((url == search_similar_url ()) && search_similar_acl (request)) request->reply = search_similar (request);
  else if ((url == search_strongs_url ()) && search_strongs_acl (request)) request->reply = search_strongs (request);
  else if ((url == search_strong_url ()) && search_strong_acl (request)) request->reply = search_strong (request);
  else if ((url == search_originals_url ()) && search_originals_acl (request)) request->reply = search_originals (request);
  else if ((url == workbench_settings_url ()) && workbench_settings_acl (request)) request->reply = workbench_settings (request);
  else if ((url == collaboration_flash_url ()) && collaboration_flash_acl (request)) request->reply = collaboration_flash (request);
  else if ((url == collaboration_open_url ()) && collaboration_open_acl (request)) request->reply = collaboration_open (request);
  else if ((url == collaboration_password_url ()) && collaboration_password_acl (request)) request->reply = collaboration_password (request);
  else if ((url == collaboration_secure_url ()) && collaboration_secure_acl (request)) request->reply = collaboration_secure (request);
  else if ((url == collaboration_direction_url ()) && collaboration_direction_acl (request)) request->reply = collaboration_direction (request);
  else if ((url == checks_settingspatterns_url ()) && checks_settingspatterns_acl (request)) request->reply = checks_settingspatterns (request);
  else if ((url == checks_settingssentences_url ()) && checks_settingssentences_acl (request)) request->reply = checks_settingssentences (request);
  else if ((url == checks_suppress_url ()) && checks_suppress_acl (request)) request->reply = checks_suppress (request);
  else if ((url == xrefs_help_url ()) && xrefs_help_acl (request)) request->reply = xrefs_help (request);
  else if ((url == xrefs_extract_url ()) && xrefs_extract_acl (request)) request->reply = xrefs_extract (request);
  else if ((url == xrefs_interpret_url ()) && xrefs_interpret_acl (request)) request->reply = xrefs_interpret (request);
  else if ((url == xrefs_translate_url ()) && xrefs_translate_acl (request)) request->reply = xrefs_translate (request);
  else if ((url == xrefs_clear_url ()) && xrefs_clear_acl (request)) request->reply = xrefs_clear (request);
  else if ((url == xrefs_insert_url ()) && xrefs_insert_acl (request)) request->reply = xrefs_insert (request);

  // Downloads
  else if ((url == index_listing_url (url)) && index_listing_acl (request, url)) request->reply = index_listing (request, url);
  
  // Client calls.
  else if ((url == sync_setup_url ()) && sync_setup_acl (request)) request->reply = sync_setup (request);
  else if ((url == sync_settings_url ()) && sync_settings_acl (request)) request->reply = sync_settings (request);
  else if ((url == sync_bibles_url ()) && sync_bibles_acl (request)) request->reply = sync_bibles (request);
  else if ((url == sync_notes_url ()) && sync_notes_acl (request)) request->reply = sync_notes (request);
  else if ((url == sync_usfmresources_url ()) && sync_usfmresources_acl (request)) request->reply = sync_usfmresources (request);
  else if ((url == sync_externalresources_url ()) && sync_externalresources_acl (request)) request->reply = sync_externalresources (request);
  
  // AJAX calls.
  else if ((url == navigation_update_url ()) && navigation_update_acl (request)) request->reply = navigation_update (request);
  else if ((url == navigation_poll_url ()) && navigation_poll_acl (request)) request->reply = navigation_poll (request);
  else if ((url == editverse_id_url ()) && editverse_id_acl (request)) request->reply = editverse_id (request);
  else if ((url == editverse_load_url ()) && editverse_load_acl (request)) request->reply = editverse_load (request);
  else if ((url == editverse_save_url ()) && editverse_save_acl (request)) request->reply = editverse_save (request);
  else if ((url == editusfm_focus_url ()) && editusfm_focus_acl (request)) request->reply = editusfm_focus (request);
  else if ((url == editusfm_id_url ()) && editusfm_id_acl (request)) request->reply = editusfm_id (request);
  else if ((url == editusfm_load_url ()) && editusfm_load_acl (request)) request->reply = editusfm_load (request);
  else if ((url == editusfm_offset_url ()) && editusfm_offset_acl (request)) request->reply = editusfm_offset (request);
  else if ((url == editusfm_save_url ()) && editusfm_save_acl (request)) request->reply = editusfm_save (request);
  else if ((url == edit_edit_url ()) && edit_edit_acl (request)) request->reply = edit_edit (request);
  else if ((url == edit_focus_url ()) && edit_focus_acl (request)) request->reply = edit_focus (request);
  else if ((url == edit_id_url ()) && edit_id_acl (request)) request->reply = edit_id (request);
  else if ((url == edit_load_url ()) && edit_load_acl (request)) request->reply = edit_load (request);
  else if ((url == edit_offset_url ()) && edit_offset_acl (request)) request->reply = edit_offset (request);
  else if ((url == edit_save_url ()) && edit_save_acl (request)) request->reply = edit_save (request);
  else if ((url == edit_styles_url ()) && edit_styles_acl (request)) request->reply = edit_styles (request);
  else if ((url == search_getids_url ()) && search_getids_acl (request)) request->reply = search_getids (request);
  else if ((url == search_replacepre_url ()) && search_replacepre_acl (request)) request->reply = search_replacepre (request);
  else if ((url == search_replacego_url ()) && search_replacego_acl (request)) request->reply = search_replacego (request);
  else if ((url == search_replacepre2_url ()) && search_replacepre2_acl (request)) request->reply = search_replacepre2 (request);
  else if ((url == search_getids2_url ()) && search_getids2_acl (request)) request->reply = search_getids2 (request);
  else if ((url == search_replacego2_url ()) && search_replacego2_acl (request)) request->reply = search_replacego2 (request);
  else if ((url == resource_get_url ()) && resource_get_acl (request)) request->reply = resource_get (request);
  else if ((url == notes_poll_url ()) && notes_poll_acl (request)) request->reply = notes_poll (request);
  else if ((url == notes_notes_url ()) && notes_notes_acl (request)) request->reply = notes_notes (request);
  else if ((url == notes_click_url ()) && notes_click_acl (request)) request->reply = notes_click (request);
  else if ((url == consistency_poll_url ()) && consistency_poll_acl (request)) request->reply = consistency_poll (request);
  else if ((url == consistency_input_url ()) && consistency_input_acl (request)) request->reply = consistency_input (request);
  else if ((url == xrefs_source_url ()) && xrefs_source_acl (request)) request->reply = xrefs_source (request);
  else if ((url == xrefs_target_url ()) && xrefs_target_acl (request)) request->reply = xrefs_target (request);
  else if ((url == xrefs_move_url ()) && xrefs_move_acl (request)) request->reply = xrefs_move (request);
  else if ((url == xrefs_next_url ()) && xrefs_next_acl (request)) request->reply = xrefs_next (request);

  // Forward the browser to the default home page.
  else {
    redirect_browser (request, index_index_url ());
  }
}
