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
        $query = $conn->prepare("select time, $tags from timeseries $res_clause order by time desc limit $maxvalues;");
        $select = $query->execute();
		
	    //$result = $query->setFetchMode(PDO::FETCH_NUM);
  //      print "[";
        $i = 0;
		$c = 0;
		 $data_points = array();
        while ($row = $query->fetch(PDO::FETCH_ASSOC)) {
  //          if ($i>1) print ",";
			$i++;
//			print '{';
//			$totalcol = $query->columnCount();
			//$point = array($row['time'], $row['hhTempIn']);
			//$point['time]
			array_push($data_points, $row);
//			for ($c = 0; $c < $totalcol; $c++) {
//				if ($c>1) print ',';
//				$c++;
//				print $row[$c];
//            print "{\"" . $row[0] . "\":\"" . $row[1] . "\"}";
//                print "\"" . $query->getColumnMeta($c)['name'] . "\":\"" . $row[$c] . "\"";
//			}
//			print '}';
        }		
//		print "]}";
//        $row = $query->fetchObject();
 
        // send the data encoded as JSON
//        echo json_encode($row);
// The SQL call returns values in reverse order, fix this.
$reversed = array();
$c = count($data_points);
for ($i = $c-1; $i >= 0; $i--) $reversed[$c-$i-1] = $data_points[$i];
echo json_encode($reversed, JSON_NUMERIC_CHECK);
        exit;
    //}
} else echo "Error: No tags specified in request";
?>