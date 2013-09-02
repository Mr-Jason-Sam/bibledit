<?php


require_once ("../bootstrap/bootstrap.php");
page_access_level (MANAGER_LEVEL);
Assets_Page::header (gettext ("Import USFM"));


$view = new Assets_View (__FILE__);
$database_bibles = Database_Bibles::getInstance();
$database_books = Database_Books::getInstance();


$success_message = "";
$error_message = "";


// The name of the Bible.
$bible = $_GET['bible'];
$view->view->bible = Filter_Html::sanitize ($bible);


// USFM data submission.
if (isset($_POST['submit'])) {
  // Submission may take long if there's a lot of data and/or the network is slow.
  ignore_user_abort (true);
  set_time_limit (0);
  $data = $_POST['data'];
  $data = trim ($data);
  if ($data != "") {
    if (Validate_Utf8::valid ($data)) {
      $datafile = tempnam (sys_get_temp_dir(), '');
      file_put_contents ($datafile, $data);
      $success_message = gettext ("Import has started. See logbook for progress.");
      $workingdirectory = dirname (__FILE__);
      $bible = escapeshellarg ($bible);
      $datafile = escapeshellarg ($datafile);
      $command = "php importcli.php $datafile $bible > /dev/null 2>&1 &";
      shell_exec ($command);
    } else {
      $error_message = gettext ("Please supply valid Unicode UTF-8 text.");
    }
  } else {
    $success_message = gettext ("Nothing was imported.");
  }
}


// File upload.
if (isset($_POST['upload'])) {
  // Upload may take long if file is big or network is slow.
  ignore_user_abort (true);
  set_time_limit (0);

  $datafile = tempnam (sys_get_temp_dir(), '');
  unlink ($datafile);
  @$datafile .= $_FILES['data']['name'];
  @$tmpfile = $_FILES['data']['tmp_name'];
  if (move_uploaded_file ($tmpfile, $datafile)) {
    $success_message = gettext ("Import has started. See logbook for progress.");
    $workingdirectory = dirname (__FILE__);
    $bible = escapeshellarg ($bible);
    $datafile = escapeshellarg ($datafile);
    $command = "php importcli.php $datafile $bible > /dev/null 2>&1 &";
    shell_exec ($command);
  } else {
    $error_message = Filter_Upload::error2text ($_FILES['data']['error']);
  }
}


$view->view->post_max_size = ini_get ("post_max_size");
$view->view->upload_max_filesize = ini_get ("upload_max_filesize");
/*
Other variables of importance are:
file_uploads
max_input_time
memory_limit
max_execution_time
For just now these are left as they are.
*/


@$view->view->success_message = $success_message;
@$view->view->error_message = $error_message;
$view->render ("import_usfm.php");
Assets_Page::footer ();


?>
