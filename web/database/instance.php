<?php

  // Page bootstrapping.
  require_once ("bootstrap/bootstrap.php");


  // The singleton Database_Instance.
  // Call it like this: $database = Database_Instance::getInstance ();
  class Database_Instance {

    public $is_connected;      // Indicates whether the database instance is connected.
    public $mysqli;            // Database object.
    private static $instance;  // Singleton object instance.
    


    private function __construct() { 

      // File with credentials to access the database server.
      require_once ("database/credentials.php");
    
      $this->is_connected = true;

      // Connect to the database server.
      // This checks the MySQL server host, the user and password to connect to the database.
      @$this->mysqli = new mysqli($database_host, $database_user, $database_pass);
      if ($this->mysqli->connect_errno) {
        $this->is_connected = false;
      }
  
      // Connect to the database.
      if ($this->is_connected) {
        $this->is_connected = $this->mysqli->select_db ($database_name);
        if (!$this->is_connected) {
          // The database was not found. It needs to be created.
          // It should use utf8 unicode by default.
          $this->mysqli->query ("CREATE DATABASE $database_name DEFAULT CHARACTER SET utf8 COLLATE utf8_unicode_ci;");
          if ($this->mysqli->errno) {
            $this->is_connected = false;
          }    
          $this->is_connected = $this->mysqli->select_db ($database_name);
        }
      }

      // If there was no connection, give a short diagnostic message.      
      if (!$this->is_connected) {
        require_once ("messages/messages.php");
        message_warning_header (gettext ("Cannot access the database"));
        message_information (gettext ("The site is not in operation at the moment. Site administrator: You can login through the default credentials or the ones that you set at installation time, and then attend to the problem."));
      }
    }


    public static function getInstance() {
      if ( empty( self::$instance ) ) {
        self::$instance = new Database_Instance();
      }
      return self::$instance;
    }

  }

?>
