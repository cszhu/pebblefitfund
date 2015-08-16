var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  // Construct URL
  var url = "sandile.ngrok.com/api/fake/leaderboard";
  // var url = "sandile.ngrok.com/api/fake/ranking";
//      + pos.coords.latitude + "&lon=" + pos.coords.longitude;

  // Send request to Sandile's server
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);
      
      //Getting the first name
      var name = json[0].name;
      console.log("name is " + name);
      
      //Getting the steps
      var steps = json[0].steps;
      console.log("steps are" + steps);
      
      // Temperature in Kelvin requires adjustment
      // var temperature = Math.round(json.main.temp - 273.15);
      // console.log("Temperature is " + temperature);

      // Conditions
//       var conditions = json.weather[0].main;      
//       console.log("Conditions are " + conditions);
      
      // Assemble dictionary using our keys
      var dictionary = {
        "KEY_NAME": name,
        "KEY_STEPS": steps
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Weather info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending weather info to Pebble!");
        }
      );
    }      
  );
}

function locationError(err) {
  console.log("Error requesting location!");
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

function getLeaderboard() {
    // Construct URL
  var url = "http://sandile.ngrok.com/api/fake/leaderboard";
  // var url = "sandile.ngrok.com/api/fake/ranking";
  
  console.log("sending off request");
  // Send request to Sandile's server
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);
      
      //Getting the first name
      var name = json[0].name;
      console.log("name is " + name);
      
      //Getting the steps
      var steps = json[0].steps;
      console.log("steps are" + steps);
      
      // Assemble dictionary using our keys
      var dictionary = {
        "KEY_NAME": name,
        "KEY_STEPS": steps
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Step info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending step info to Pebble!");
        }
      );
    }      
  );
  
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!!");
    console.log("trying to get leaderboard");
    // Get the initial leaderboard
    getLeaderboard();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    getWeather();
  }                     
);
