<?php
if(!empty($_POST))
{
	//database settings
	include "db_config.php";

	foreach($_POST as $field_name => $val)
	{
		//clean post values
		$conn = new PDO("mysql:host=$server;dbname=$database", $username, $password);
		$field_id = strip_tags(trim($field_name));
		$val = strip_tags(trim($conn->quote($val)));
		if(!empty($field_id) && !empty($val))
		{
			//update the values
			$sql = "INSERT INTO settings (id, value) VALUES('$field_id', $val) ON DUPLICATE KEY UPDATE value=$val";
			try {
			
			$conn ->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
			
			// Prepare statement
			$stmt = $conn->prepare($sql);
	
			// execute the query
			$stmt->execute();
			//mysql_query("UPDATE settings SET value = '$val' WHERE name = $field_id") or mysql_error();
			// echo a message to say the UPDATE succeeded
			echo $stmt->rowCount() . " records UPDATED successfully";
			//echo "Updated";
			}
			catch(PDOException $e)
			{
				echo $sql . "<br>" . $e->getMessage();
			}

			$conn = null;
		} else {
			echo "Invalid Requests";
		}
	}
} else {
	echo "Invalid Requests";
}
?>