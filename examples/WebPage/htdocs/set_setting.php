<?php
// Insert or update the supplied JSON name:value pair in the settings table.
if(!empty($_POST))
{
	// Database settings
	include "db_config.php";

	foreach($_POST as $field_name => $val)
	{
		// Clean post values
		$conn = new PDO("mysql:host=$server;dbname=$database", $username, $password);
		$field_id = strip_tags(trim($field_name));
		$val = strip_tags(trim($conn->quote($val)));
		if(!empty($field_id) && !empty($val))
		{
			// Update the values
			$sql = "INSERT INTO settings (id, value) VALUES('$field_id', $val) ON DUPLICATE KEY UPDATE value=$val";
			try {			
				$conn ->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
				
				// Prepare statement
				$stmt = $conn->prepare($sql);
		
				// Execute the query
				$stmt->execute();

				// Echo a message to say the UPDATE succeeded
				echo $stmt->rowCount() . " records UPDATED successfully";
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