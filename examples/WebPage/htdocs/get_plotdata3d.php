<?php
//database settings
include "db_config.php";

// check for AJAX request
if (isset($_GET['tags'])) {
    //if ($_GET['action'] == 'fetch') {

		$tags = $_GET['tags'];
	
	    // Get the requested resolution (if not requested then return lowest resolution)
	    $res_clause = '';
		if (isset($_GET['resolution'])) {
			$resolution = $_GET['resolution'];
			$res_clause = "where $resolution = 1";
		}

	    // Get the max number of values
	    $maxvalues = 60;
		if (isset($_GET['maxvalues'])) {
			$maxvalues = $_GET['maxvalues'];
		}
	
   	    // tell the browser what's coming
        header('Content-type: application/json');
 
        // open database connection
        $conn = new PDO("mysql:host=$server;dbname=$database", $username, $password);
 
        // use prepared statements!
        $query = $conn->prepare("select $tags, FLOOR(UNIX_TIMESTAMP(time)) as UTC from timeseries $res_clause order by time desc limit $maxvalues;");
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