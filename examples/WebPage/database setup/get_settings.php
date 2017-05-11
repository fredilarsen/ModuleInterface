<?php
//database settings
include "db_config.php";

// tell the browser what's coming
header('Content-type: application/json');

// open database connection
$conn = new PDO("mysql:host=$server;dbname=$database", $username, $password);

// use prepared statements!
$query = $conn->prepare('select * from settings');
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