<?php
require '../phpSpark/phpSpark.class.php';

$accessToken = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
$deviceID = "XXXXXXXXXXXXXXXXXXXXXXXX"

$spark = new phpSpark();

$spark->setTimeout(5);
$spark->setAccessToken($accessToken);

$spark->callFunction($deviceID, 'function', 'flamerad,100');

sleep(2);

$spark->callFunction($deviceID, 'function', 'flamerad,40');