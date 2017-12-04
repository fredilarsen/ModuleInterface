<?php
// Database settings
include "db_config.php";

// Quoting of identifiers (http://php.net/manual/en/pdo.quote.php#112169)
function quoteIdent($field) {
    return "`".str_replace("`","``",$field)."`";
}

// Check for AJAX request
if (isset($_GET['tags'])) {
	// Open database connection
	$conn = new PDO("mysql:host=$server;dbname=$database;charset=utf8", $username, $password);
		
	// Get and sanitize tag name list
	$tags = $_GET['tags'];
	$tagArray = explode(",", $tags);
	foreach ($tagArray as $value) 
		$cleanTagArray [] = quoteIdent(str_replace("'", "", $conn->quote($value)));
	$tags = implode(",", $cleanTagArray);

	// Get the requested resolution (if not requested then return most detailed resolution)
	$res_clause = '';
	if (isset($_GET['resolution'])) {
		$resolution = $_GET['resolution'];
		// Sanitize resolution column name
		if (in_array($resolution, ['scan1m','scan10m','scan1h','scan1d']))
		  $res_clause = "where $resolution = 1";
	}

	// Get the max number of values
	$maxvalues = 60;
	if (isset($_GET['maxvalues'])) {
		$maxvalues = intval($_GET['maxvalues']);
	}

	// Tell the browser what's coming
	header('Content-type: application/json');

	// Use prepared statements!
	$query = $conn->prepare("select time, $tags from timeseries $res_clause order by time desc limit :maxvalues;");
	$query->bindValue(':maxvalues', $maxvalues, PDO::PARAM_INT);
	$select = $query->execute();
	
	$i = 0;
	$c = 0;
	$data_points = array();
	while ($row = $query->fetch(PDO::FETCH_ASSOC)) {
		$i++;
		array_push($data_points, $row);
	}		

	// The SQL call returns values in reverse order, fix this.
	$reversed = array();
	$c = count($data_points);
	for ($i = $c-1; $i >= 0; $i--) $reversed[$c-$i-1] = $data_points[$i];
	echo json_encode($reversed, JSON_NUMERIC_CHECK);
	exit;
} else echo "Error: No tags specified in request";
?>