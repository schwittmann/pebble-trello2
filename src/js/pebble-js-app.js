
var MESSAGE_TYPE_BOARDS	= 0;
var MESSAGE_TYPE_INIT	= 1;

var loadedInit = false;
var globalData = {};



function makeRequest(urlpath, success, fail, verb) {
  function decodeUtf8(utftext) {
    var minimalMappingUtf8ToIso8859 = {
      228:'ae',
      196:'Ae',
      252:'ue',
      220:'Ue',
      246:'oe',
      214:'Oe',
      223:'ss'
    };
    var ret = "";
    for(var i=0; i<utftext.length; ++i) {
      if(utftext.charCodeAt(i) in minimalMappingUtf8ToIso8859) {
        ret += minimalMappingUtf8ToIso8859[utftext.charCodeAt(i)];
      } else {
        ret += utftext[i];
      }
      if (utftext.charCodeAt(i) > 127) {
        console.log("charcode:"+utftext.charCodeAt(i));
      }
    }
    return ret;
  }
  var req = new XMLHttpRequest();
  if(!verb) {
    verb = "get";
  }
  req.open(verb, 'https://api.trello.com/1/'+urlpath+'&key=e3227833b55cbe24bfedd05e5ec870dd&token='+localStorage.getItem("token"));
  req.onload = function(e) {
    if (req.readyState != 4)
        return;
    if(req.status == 200) {
      var decoded;
      console.log("Pre decode:"+window.btoa(req.responseText));
      try {
        decoded = decodeUtf8(req.responseText);
      } catch(err) {
        console.log("decoding failed:"+err);
        return;
      }
      console.log("got decoded:"+decoded);
      success(JSON.parse(decoded));
    } else {
      console.log("Http request failed :("+ req.responseText);
      fail(req.responseText, req.status);
    }
  };
  req.send(null);
}



Pebble.addEventListener("ready",
	function(e) {
		console.log("Javascript got ready");
    var hasToken = localStorage.getItem("token")?1:0;

    if(hasToken) {
      console.log("ready: token available, loading boards")
      loadBoards();
    } else {
      var msg = {};
      msg.type = MESSAGE_TYPE_INIT;
      msg.value = hasToken;
      Pebble.sendAppMessage(msg);
    }
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
      loadBoards();
    }
    return;
  }
  
  if(localStorage.getItem("token") == configuration.token) {
    console.log("Got already known token");
  }
  localStorage.setItem("token", configuration.token);
  console.log("Set token: "+configuration.token);
  loadBoards();
});

Pebble.addEventListener("appmessage",
	function(e) {
		console.log("JS: got message");
		console.log(JSON.stringify(e.payload));
	}
);


function loadBoards() {
  loadedInit = true;
  makeRequest('members/me?fields=username&boards=open&board_lists=open', loadedUser, loadingFailed);
}


function addData(msg, data) {
  msg.numElements1 = data.length;
  var msgIndex = 0;
  for(var key in data) {
    var array = data[key];
    msg[msgIndex++] = key;
    msg[msgIndex++] = array.length;
    for(var i in array) {
      var element = array[i];
      msg[msgIndex++] = element;
    }
  }
}
function loadedUser(user) {
  globalData.user = user;

  var data = {};
  for(var i=0; i<user.boards.length;++i) {
    data[user.boards[i].name] = user.boards[i].lists.map(function(e){return e.name});
  }

  var msg = {};
  msg.type = MESSAGE_TYPE_BOARDS;
  addData(msg, data);
  
  Pebble.sendAppMessage(msg);

//    selectedBoard(user.boards[e.itemIndex]);
}

function loadingFailed(txt, code) {
  console.log("Loading failed "+code+": "+txt);
}