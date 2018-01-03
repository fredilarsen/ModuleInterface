<?php
// Return all rows from the currentvalues table.
try {
  //database settings
  include "db_config.php";

  // tell the browser what's coming
  header('Content-type: application/json');

  // open database connection
  $conn = new PDO("mysql:host=$server;dbname=$database;charset=utf8", $username, $password);

  // use prepared statements!
  $query = $conn->prepare('SELECT id, value, FLOOR(UNIX_TIMESTAMP(modified)) as UTC FROM currentvalues ORDER BY id;');
  $query->execute();

  $result = $query->setFetchMode(PDO::FETCH_NUM);
  print "{\"SERVERTIME\":" . time();
  $i = 0;
  $c = 0;
  $last_modified = 0; // Time of most recently modified value
  while ($row = $query->fetch()) {
    print ",\"" . $row[0] . "\":\"" . $row[1] . "\"";
    if ($row[2] > $last_modified) $last_modified = $row[2];
  }
  print ",\"UTC\":\"" . $last_modified . "\"}";
} catch (PDOException $e) {
  print "Error!: " . $e->getMessage() . "<br/>";
  die();
}
?>

