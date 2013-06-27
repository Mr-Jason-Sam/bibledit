<?php
require_once ("../bootstrap/bootstrap.php");
page_access_level (ADMIN_LEVEL);
Assets_Page::header (gettext ("Collaboration"));
$smarty = new Smarty_Bibledit (__FILE__);
$object = $_GET ['object'];
$smarty->assign ("object", $object);
$database_config_user = Database_Config_User::getInstance();
$url = $database_config_user->getRemoteRepositoryUrl ($object);
$directory = Filter_Git::git_directory ($object);
$smarty->display("collaboration_repo_data.tpl");
Assets_Page::footer ();
?>
