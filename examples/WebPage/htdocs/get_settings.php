<?php
// Database settings
include "db_config.php";

// Tell the browser what's coming
header('Content-type: application/json');

// Retrieve only settings starting with specified prefix?
$prefix = null;
if (!empty($_GET)) $prefix = array_key_exists('prefix', $_GET) ? $_GET['prefix'] : null;

// Open database connection
$conn = new PDO("mysql:host=$server;dbname=$database;charset=utf8", $username, $password);

// Use prepared statements!
$sql = "select id, value from settings";
if ($prefix != null) $sql = $sql . " where id like :prefix";
$query = $conn->prepare($sql);
$query->execute( array(':prefix' => "$prefix%"));

$result = $query->setFetchMode(PDO::FETCH_NUM);
print "{\"UTC\":\"" . time(0) . "\"";
$i = 0;
while ($row = $query->fetch()) {
	print ",";
	$i++;
	print "\"" . $row[0] . "\":\"" . $row[1] . "\"";
}
print "}";
exit;
?>