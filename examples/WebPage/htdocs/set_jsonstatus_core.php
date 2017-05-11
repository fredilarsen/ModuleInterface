<?php
if(isset($_SERVER["CONTENT_TYPE"]) && strpos($_SERVER["CONTENT_TYPE"], "application/json") !== false) {
    $_POST = array_merge($_POST, (array) json_decode(trim(file_get_contents('php://input')), true));
	// Apply variable write whitelist for the timeseries table
	foreach(file('variable_write_list.json') as $line) $variables[trim($line)] = "";
	$_POST = array_intersect_key($_POST, $variables);
}
if(!empty($_POST)) {
	// database settings
	include "db_config.php";
	
	$sql = "INSERT INTO timeseries SET ";

	// Support utdating last row instead of inserting
	if ($update) $sql = "UPDATE timeseries SET ";
	
	$first = true;
	foreach($_POST as $field_name => $val)
	{
		// clean post values
		$field_id = strip_tags(trim($field_name));
		$val = strip_tags(trim($val));
		if(!empty($field_id))
		{
			//echo $field_id . $val;
			// update the values
			if ($first) $first = false;
			else {
				$sql = $sql . ',';
			}
			$sql = $sql . $field_id . " = '" . $val . "'";
		}
	}
	//echo $sql;
	try {
		$conn = new PDO("mysql:host=$server;dbname=$database", $username, $password);
		$conn ->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

		// Call simple statement to get latest time stamp present
		if ($update) {
			$stmt = $conn->prepare("SELECT MAX(id) FROM timeseries;");
			$stmt->execute();
			$maxid = $stmt->fetch(PDO::FETCH_BOTH);
			$sql = $sql . " WHERE id = " . $maxid[0];
			$stmt = null;
		}
		$sql = $sql . ';';
		
		// Prepare statement
		$stmt = $conn->prepare($sql);

		// Execute the query
		$stmt->execute();

		// Echo a message to say the UPDATE succeeded
		//echo $stmt->rowCount() . " records UPDATED successfully";
	}
	catch(PDOException $e)
	{
		echo $sql . "<br>" . $e->getMessage();
	}

	$conn = null;
} else {
	echo "Invalid request (empty post)";
}
?>