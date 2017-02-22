<?php
header("Access-Control-Allow-Origin: *");

if (!empty($_GET)) {
    $domain = "https://maps.googleapis.com/maps/api/geocode/xml";
    $address = "address=" . urlencode($_GET["streetAddress"]) . "," . urlencode($_GET["city"]) . "," . $_GET["states"];
    $key = "key=" . "AIzaSyCu7A4_Grgv83Aa6_xJQpfVVapq20Ebgwc";
    $query = $address . "&" . $key;
    $request = file_get_contents($domain."?".$query);
    $xml =simplexml_load_string($request) or die("Error: cannot create object");
    $lat = $xml->result[0]->geometry->location->lat;
    $lng = $xml->result[0]->geometry->location->lng;
    $domain = "https://api.forecast.io/forecast/4a8f4cc26c31f272bcbf8c5ebb2b262e/";
    $query = $lat.",".$lng."?units=".$_GET['degree']."&exclude=flags";
    $request = file_get_contents($domain . $query);
    echo $request;
}
?>