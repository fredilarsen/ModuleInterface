<?php
//database settings
include "db_config.php";

// Quoting of identifiers (http://php.net/manual/en/pdo.quote.php#112169)
function quoteIdent($field) {
    return "`".str_replace("`","``",$field)."`";
}

// check for AJAX request
if (isset($_GET['tags'])) {
    //if ($_GET['action'] == 'fetch') {

	// open database connection
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

	// tell the browser what's coming
	header('Content-type: application/json');

	// use prepared statements!
	$query = $conn->prepare("select $tags, FLOOR(UNIX_TIMESTAMP(time)) as UTC from timeseries $res_clause order by time desc limit :maxvalues;");
	$query->bindValue(':maxvalues', $maxvalues, PDO::PARAM_INT);
	$select = $query->execute();

	$i = 0;
	$c = 0;
	$x = array();
	$y = array();
	$f = array();
	$totalcol = 0;
		
	while ($row = $query->fetch(PDO::FETCH_BOTH)) {
		$totalcol = $query->columnCount();
		$x[] = $row["UTC"];
		$v = array();
		for ($c = 0; $c < $totalcol -1; $c++) $v[$c] = $row[$c]; // -1 to avoid UTC at the end
		$f[] = $v;
	}
	
	// Create an y array (values along y axis)
	for ($c = 0; $c < $totalcol-1; $c++) $y[$c] = $c; // -1 to avoid UTC at the end
		
	// The SQL call returns values in reverse order, fix this.
	$x = array_reverse($x);
	$f = array_reverse($f);
	$xdata["NAME"] = "UTC"; $xdata["VALUES"] = $x;
	$ydata["NAME"] = "Val#"; $ydata["VALUES"] = $y;
	$fdata["NAME"] = "F"; $fdata["VALUES"] = $f;
	$allf = array();
	$allf[] = $fdata; // Just one scalar field in this case, but format supports multiple values
	$data["X"] = $xdata;
	$data["Y"] = $ydata;
	$data["F"] = $allf;
	echo json_encode($data, JSON_NUMERIC_CHECK);
    exit;
  //}
} else echo "Error: No tags specified in request";
?>