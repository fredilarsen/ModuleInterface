<?php
// Save all the name:value pairs in the supplied JSON to the currentvalues table.

if(isset($_SERVER["CONTENT_TYPE"]) && strpos($_SERVER["CONTENT_TYPE"], "application/json") !== false) {
    $_POST = array_merge($_POST, (array) json_decode(trim(file_get_contents('php://input')), true));
}
if(!empty($_POST)) {
	// database settings
	include "db_config.php";
	
	$sql = "INSERT INTO settings (id, value) VALUES ";

	try {
		$conn = new PDO("mysql:host=$server;dbname=$database;charset=utf8", $username, $password);
		$conn ->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
		
		$first = true;
		foreach($_POST as $field_name => $val)
		{
			// clean post values
			$field_id = $conn->quote(strip_tags(trim($field_name)));
			$val = $conn->quote(strip_tags(trim($val)));
			if(!empty($field_id))
			{
				// update the values
				if ($first) $first = false;	else $sql = $sql . ",";
				$sql = $sql . "(" . $field_id . "," . $val . ")";
			}
		}
		$sql = $sql . " ON DUPLICATE KEY UPDATE value = VALUES(value);";
		
		// Prepare statement
		$stmt = $conn->prepare($sql);

		// Execute the query
		$stmt->execute();
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