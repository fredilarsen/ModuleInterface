<?php
//database settings
include "db_config.php";

// tell the browser what's coming
header('Content-type: application/json');

// open database connection
$conn = new PDO("mysql:host=$server;dbname=$database", $username, $password);

// use prepared statements!
$query = $conn->prepare('select *, FLOOR(UNIX_TIMESTAMP()) as SERVERTIME, FLOOR(UNIX_TIMESTAMP(time)) as UTC from timeseries order by time desc limit 1;');
$select = $query->execute();

$result = $query->setFetchMode(PDO::FETCH_NUM);
print "{";
$i = 0;
$c = 0;
if ($row = $query->fetch(PDO::FETCH_BOTH)) {
	$totalcol = $query->columnCount();
	for ($c = 1; $c < $totalcol; $c++) {
	  if ($c>1) print ",";
	  print "\"" . $query->getColumnMeta($c)['name'] . "\":\"" . $row[$c] . "\"";
	}
}		
print "}";
exit;
?>