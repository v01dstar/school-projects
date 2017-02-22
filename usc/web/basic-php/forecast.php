<html>
    <head>
        <meta charset="utf-8">
        <title>Forecast</title>
        <link rel="stylesheet" href="css/forecast.css">
        <script type="text/javascript">
            function checkValidation() {
                var missing_field = [];
                if (document.getElementById("street_address").value == null || document.getElementById("street_address").value == "") {
                    missing_field.push("street address");
                }
                if (document.getElementById("city").value == null || document.getElementById("city").value == "") {
                    missing_field.push("city");
                }
                if (document.getElementById("states").value == null || document.getElementById("states").value == "" || document.getElementById("states").value == "select your state ...") {
                    missing_field.push("states");
                }
                if (!(document.getElementById("degree_f").checked || document.getElementById("degree_c").checked)) {
                    missing_field.push("degree");
                }
                if (missing_field.length == 0) {
                    return true;
                }
                var err_msg = "Please enter value for: ";
                for (i = 0; i < missing_field.length; i++) {
                    err_msg += missing_field[i];
                    if (i == missing_field.length - 1) {
                        err_msg += ".";
                    } else {
                        err_msg += ", ";
                    }
                }
                alert(err_msg);
                return false;
            }
            
            function resetPage() {
                var myForm = document.getElementById("queryForm");
                myForm.street_address.value = "";
                myForm.city.value = "";
                myForm.states.selectedIndex = 0;
                document.getElementById("degree_c").checked = false;
                document.getElementById("forecast").innerHTML = "";
                return true;
            }
        </script>
    </head>
    <body>
        <h1>Forcast Search</h1>
        <form id="queryForm" action="" method="post" onsubmit="return checkValidation()">
            <fieldset>
                <table>
                    <tr>
                        <td>
                            <label for="street_address">Street Address:*</label>
                        </td>
                        <td>
                            <input type="text" name="street_address" id="street_address" value="<?php echo isset($_POST['street_address'])? $_POST['street_address'] : '' ?>">
                        </td>
                    </tr>
                    <tr>
                        <td>
                            <label for="city">City:*</label>
                        </td>
                        <td>
                            <input type="text" name="city" id="city" value="<?php echo isset($_POST['city'])? $_POST['city'] : '' ?>">
                        </td>
                    </tr>
                    <tr>
                        <td>
                            <label for="states">States:*</label>
                        </td>
                        <td>
                            <select name="states" id="states">
                                <option <?php echo isset($_POST['states']) ? 'selected' : ''?> >select your state ...</option>
                                <?php
                                    $states_map_file = file_get_contents("data/states.json");
                                    $states_map = json_decode($states_map_file, true);
                                    foreach ($states_map as $state => $abbrev) {
                                        echo "<option value='$abbrev'" . (isset($_POST["states"]) && ($_POST["states"] == $abbrev) ? " selected" : "") . ">$state</option>";
                                    }
                                ?>
                            </select>
                        </td>
                    </tr>
                    <tr>
                        <td>
                            <label for="degree">Degree:*</label>
                        </td>
                        <td>
                            <input type="radio" name="degree" id="degree_f" value="us" checked>Fahrenheit
                            <input type="radio" name="degree" id="degree_c" value="si" <?php echo (isset($_POST["degree"]) && ($_POST['degree'] == "si")) ?  "checked" : "" ?>>Celsius
                        </td>
                    </tr>
                    <tr>
                        <th colspan="2">
                            <input type="submit" name="submit" value="Submit">
                            <input type="button" name="clear" value="Clear" onclick="resetPage()" >
                        </th>
                    </tr>
                    <tr>
                        <td>
                            <div class="tips"><i>*-Mandatory fields.</i></div>
                        </td>
                    </tr>
                    <tr>
                        <th colspan="2">
                            <a href="http://forecast.io/">Powered by Forecast.io</a>
                        </th>
                    </tr>
                </table>
            </fieldset>
        </form>
        <br/>
        <br/>
        <br/>
        <br/>
        <?php if( isset($_POST["submit"])):
                $domain = "https://maps.googleapis.com/maps/api/geocode/xml";
                $address = "address=" . urlencode($_POST["street_address"]) . "," . urlencode($_POST["city"]) . "," . $_POST["states"];
                $key = "key=" . "AIzaSyCu7A4_Grgv83Aa6_xJQpfVVapq20Ebgwc";
                $query = $address . "&" . $key;
                $request = file_get_contents($domain."?".$query);
                $xml =simplexml_load_string($request) or die("Error: cannot create object");
                $lat = $xml->result[0]->geometry->location->lat;
                $lng = $xml->result[0]->geometry->location->lng;
                $domain = "https://api.forecast.io/forecast/4a8f4cc26c31f272bcbf8c5ebb2b262e/";
                $query = $lat.",".$lng."?units=".$_POST['degree']."&exclude=flags";
                $request = file_get_contents($domain . $query);
                $json = json_decode($request, true);
                $weather_map_file = file_get_contents("data/weather.json");
                $weather_map = json_decode($weather_map_file, true);
        ?>
        <div id="forecast">
        <fieldset>
            <table>
                <tr>
                    <th colspan="2">
                        <?php echo $json["currently"]["summary"] ?>
                        <br/>
                        <?php 
                            echo round($json["currently"]["temperature"]);
                            echo $_POST['degree'] == "us"? "°F" : "°C";
                        ?>
                        <br/>
                        <img src="images/<?php echo $weather_map[$json['currently']['icon']] ?>" alt="<?php echo $json['currently']['summary'] ?>">
                    </th>
                </tr>
                <tr>
                    <th colspan="2"><br/></th>
                </tr>
                <tr>
                    <td>Precipitation:</td>
                    <td>
                        <?php
                            $preci = $json["currently"]["precipIntensity"];
                            switch (true) {
                                case (($_POST['degree'] == "us" && $preci >= 0 && $preci < 0.002) || ($_POST['degree'] == "si" && $preci >= 0 && $preci < 0.0508)) :
                                    echo "None";
                                    break;
                                case (($_POST['degree'] == "us" && $preci >= 0.002 && $preci < 0.017) || ($_POST['degree'] == "si" && $preci >= 0.0508 && $preci < 0.4318)):
                                    echo "Very Light";
                                    break;
                                case (($_POST['degree'] == "us" && $preci >= 0.017 && $preci < 0.1) || ($_POST['degree'] == "si" && $preci >= 0.4318 && $preci < 2.54)):
                                    echo "Light";
                                    break;
                                case (($_POST['degree'] == "us" && $preci > 0.1 && $preci < 0.4) || ($_POST['degree'] == "si" && $preci >= 2.54 && $preci < 10.16)):
                                    echo "Moderate";
                                    break;
                                default:
                                    echo "Heavy";
                                    break;
                            }
                        ?>
                    </td>
                </tr>
                <tr>
                    <td>Chance of Rain:</td>
                    <td>
                        <?php echo ($json["currently"]["precipProbability"] * 100)."%"; ?>
                    </td>
                </tr>
                <tr>
                    <td>Wind Speed:</td>
                    <td>
                        <?php echo round($json["currently"]["windSpeed"]). ($_POST['degree'] == "us"?" mph": " Mps"); ?>
                    </td>
                </tr>
                <tr>
                    <td>Dew Point:</td>
                    <td>
                        <?php echo round($json["currently"]["dewPoint"]).($_POST['degree'] == "us"? "°F": "°C"); ?>
                    </td>
                </tr>
                <tr>
                    <td>Humidity:</td>
                    <td>
                        <?php echo ($json["currently"]["humidity"] * 100)."%"; ?>
                    </td>
                </tr>
                <tr>
                    <td>Visibility:</td>
                    <td>
                        <?php echo round($json["currently"]["visibility"]).($_POST['degree'] == "us"? " mi": " KM"); ?>
                    </td>
                </tr>
                <tr>
                    <td>Sunrise:</td>
                    <td>
                        <?php
                            date_default_timezone_set($json["timezone"]);
                            echo date("g:i A", $json["daily"]["data"][0]["sunriseTime"]);
                        ?>
                    </td>
                </tr>
                <tr>
                    <td>Sunset:</td>
                    <td>
                        <?php
                            date_default_timezone_set($json["timezone"]);
                            echo date("g:i A", $json["daily"]["data"][0]["sunsetTime"]);
                        ?>
                    </td>
                </tr>
            </table>
            <br/>
            <br/>
        </fieldset>
        </div>
        <?php endif; ?>
    </body>
</html>
