<?php
//database settings
include "db_config.php";

// tell the browser what's coming
header('Content-type: application/json');

// Retrieve only settings starting with specified prefix
$prefix = $_GET['prefix'];

// open database connection
$conn = new PDO("mysql:host=$server;dbname=$database", $username, $password);

// use prepared statements!
$query = $conn->prepare("select id, value from settings where id like '$prefix%'");
$query->execute();

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