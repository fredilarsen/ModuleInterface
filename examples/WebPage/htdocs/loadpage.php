<?php
if (isset($_GET['page'])) {
  $page = $_GET['page'];
  $filehead = "./" . $page . "_head.html";
  $filebody = "./" . $page . ".html";
  $headexists = (file_exists($filehead) && is_readable($filehead));
  $bodyexists = (file_exists($filebody) && is_readable($filebody));
}
if (!isset($page) || !$bodyexists) {
  header('HTTP/1.0 404 Not Found');
  echo "404 Page not found";
  die;
}
?>
<!DOCTYPE html>
  <head>
  <?php
    include("homecontrol_head.html");
    if ($headexists) echo file_get_contents($filehead);
  ?>
  </head>
  <body>
  <?php
    include("homecontrol_body.html");
    echo file_get_contents($filebody);
  ?>
  </body>
</html>
