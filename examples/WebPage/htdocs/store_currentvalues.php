<?php
// Take a snapshot of values in the currentvalues table and insert it as a new row in the timeseries table.

// Database settings
include "db_config.php";

// Open database connection
$conn = new PDO("mysql:host=$server;dbname=$database;charset=utf8", $username, $password);

// Get existing columns from timeseries table
$query = $conn->prepare("SELECT COLUMN_NAME FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_NAME = 'timeseries';");
$query->setFetchMode(PDO::FETCH_NUM);
$query->execute();
$columnlist = array();
while ($row = $query->fetch()) {
	$columnlist[$row[0]] = '';
}
$query->closeCursor();
$query = null;

// Build the statement for inserting recent values into the timeseries table
$sql = "INSERT INTO timeseries SET ";
$query = $conn->prepare("SELECT id, value FROM currentvalues;"); // WHERE UNIX_TIMESTAMP(modified) + 60 > UNIX_TIMESTAMP()
$query->execute();
$query->setFetchMode(PDO::FETCH_NUM);
$first = true;
while ($row = $query->fetch()) {
	if (!array_key_exists($row[0], $columnlist)) {
		//print "Not stored in timeseries: " . $row[0];
		continue; // Not an existing column in the timeseries database
	}
	if ($first) $first = false; else $sql = $sql . ',';
	$sql = $sql . $row[0] . "=\"" . $row[1] . "\"";
}
$query->closeCursor();
$sql = $sql . ";";

// Prepare statement
$stmt = $conn->prepare($sql);

// Execute the query
$stmt->execute();
?>