
var MESSAGE_TYPE_BOARDS	= 0;
var MESSAGE_TYPE_INIT	= 1;

Pebble.addEventListener("ready",
	function(e) {
		console.log("Javascript got ready");
		var msg = {};
		msg.type = MESSAGE_TYPE_INIT;
		msg.value = localStorage.getItem("token")?1:0;
    Pebble.sendAppMessage(msg);
		/*
		var msg = {};
		msg.type = 0;
		msg.elements = 50;
		for(var i=0; i<msg.elements;++i)
			msg[i] = "i="+i;
		
		Pebble.sendAppMessage(msg);
*/
	}
);

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL('https://trello.com/1/authorize?callback_method=fragment&scope=read,write&expiration=never&name=Pebble&key=e3227833b55cbe24bfedd05e5ec870dd&return_url=https://pebble-trello.appspot.com');
});

Pebble.addEventListener("webviewclosed", function (e) {
  var decoded = decodeURIComponent(e.response);
  console.log("Decoded:"+decoded);
  var configuration;
  try {
    configuration = JSON.parse(decoded);
  } catch(err) {
    console.log("Failed to parse:"+err);
    // no configuration needed?
    if (localStorage.getItem("token") && !loadedInit) {
      console.log("token available, okay...");
      loadStuff();
    }
    return;
  }
  
  if(localStorage.getItem("token") == configuration.token) {
    console.log("Got already known token");
  }
  localStorage.setItem("token", configuration.token);
  console.log("Set token: "+configuration.token);
  loadStuff();
});

Pebble.addEventListener("appmessage",
	function(e) {
		console.log("JS: got message");
		console.log(JSON.stringify(e.payload));
	});
