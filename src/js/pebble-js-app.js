
var MESSAGE_TYPE_BOARDS	= 0;
var MESSAGE_TYPE_INIT	= 1;
var MESSAGE_TYPE_SELECTED_LIST = 2;
var MESSAGE_TYPE_CARDS = 3;
var MESSAGE_TYPE_SELECTED_CHECKLIST = 4;
var MESSAGE_TYPE_CHECKLIST = 5;
var MESSAGE_TYPE_SELECTED_ITEM = 6;
var MESSAGE_TYPE_ITEM_STATE_CHANGED = 7;
var MESSAGE_TYPE_HTTP_FAIL          = 8

var loadedInit = false;
var globalData = {};


var DEBUG = false;

var DEBUG_DATA = {
  'lists/544170754ca9faba842252bf/cards?fields=id,name,checklists&checklists=all': [{"id":"544170b7867dffe084b8ce56","name":"Duisburg","checklists":[{"id":"544170bce372301185330784","name":"Checklist","idBoard":"544170754ca9faba842252be","idCard":"544170b7867dffe084b8ce56","pos":16384,"checkItems":[{"id":"544394af4a241bafe46260e6","name":"Wasserhaehne","nameData":null,"pos":114688,"state":"incomplete"},{"id":"544bf4b10658e8a0ff1cd4f1","name":"Personenwage","nameData":null,"pos":114576,"state":"incomplete"},{"id":"5451550605e92341cf339ab3","name":"Weisswaschmittel","nameData":null,"pos":114240,"state":"incomplete"},{"id":"5457fa6adc7744f3fb08b071","name":"Kinderpflaster","nameData":{"emoji":{}},"pos":57120,"state":"complete"},{"id":"5459e31ac22d1b6ac5111a8f","name":"Brotaufstrich","nameData":null,"pos":85680,"state":"complete"},{"id":"5459e32535d5117bb4db642c","name":"Kaffee Pads","nameData":null,"pos":147456,"state":"complete"},{"id":"545a75cda018f677641a249e","name":"Muell, swirl 35l","nameData":null,"pos":163840,"state":"incomplete"}]}]},{"id":"544170c8253f4ec05eddd2a8","name":"Wohnung Koeln","checklists":[{"id":"544decbe9bdc693f924af049","name":"Checklist","idBoard":"544170754ca9faba842252be","idCard":"544170c8253f4ec05eddd2a8","pos":16384,"checkItems":[{"id":"544decc35dbcd398d4a3bb1f","name":"Duschgel","nameData":null,"pos":16384,"state":"incomplete"},{"id":"544decc797ee2c2292a13018","name":"Zahnpasta","nameData":null,"pos":32768,"state":"incomplete"}]}]},{"id":"5442a51f51648779c7ef9a3e","name":"Einkauf Chris event","checklists":[{"id":"5442a525bea253e8e4b27d40","name":"Checklist","idBoard":"544170754ca9faba842252be","idCard":"5442a51f51648779c7ef9a3e","pos":16384,"checkItems":[{"id":"5442a5348660bc59a9d3bb5d","name":"2x Blanchet trocken","nameData":null,"pos":16384,"state":"incomplete"},{"id":"5442a5444b3ff9c4b577ef5d","name":"2x blanchet halbtrocken","nameData":null,"pos":32768,"state":"incomplete"},{"id":"5442a54c3d0e94649884dff4","name":"3X Zucchini","nameData":{"emoji":{}},"pos":49152,"state":"incomplete"},{"id":"5442a58cbe06e7e2bfbacbdb","name":"8 Tomaten","nameData":null,"pos":65536,"state":"incomplete"},{"id":"5442a5c91953d5c0c71f0adc","name":"2 Schafskaese","nameData":null,"pos":81920,"state":"incomplete"},{"id":"5442a5f139f0cc2af4e3a5a0","name":"700g fussili","nameData":null,"pos":98304,"state":"incomplete"},{"id":"5442a62ba873c76fc77f1fca","name":"1 Sahneersatz","nameData":null,"pos":114688,"state":"incomplete"},{"id":"5442a650f388871d85c84a53","name":"Safari","nameData":{"emoji":{}},"pos":131072,"state":"incomplete"},{"id":"5442a66468ba902c2900acef","name":"Wodka","nameData":null,"pos":147456,"state":"incomplete"},{"id":"5442a66ffbeb3a7bb54bd424","name":"Orangensaft","nameData":null,"pos":163840,"state":"incomplete"},{"id":"5442a671942d902885be8cd5","name":"Energie","nameData":null,"pos":180224,"state":"incomplete"}]}]}]
,
  'members/me?fields=username&boards=open&board_lists=open': 
  {"id":"54416f2360cce3ba8294d79b","username":"lrnzschwttmnn","boards":[{"id":"5443d7905521141e85512082","name":"Essen und Getraenke","closed":false,"idOrganization":null,"pinned":false,"lists":[{"id":"5443d7905521141e85512083","name":"Wein","closed":false,"idBoard":"5443d7905521141e85512082","pos":16384,"subscribed":false},{"id":"5443d7905521141e85512084","name":"Whisky","closed":false,"idBoard":"5443d7905521141e85512082","pos":32768,"subscribed":false},{"id":"544d52d78e03372da51d8ada","name":"Essen","closed":false,"idBoard":"5443d7905521141e85512082","pos":114688,"subscribed":false}]},{"id":"544170754ca9faba842252be","name":"L und J","closed":false,"idOrganization":null,"pinned":false,"lists":[{"id":"544170754ca9faba842252bf","name":"Einkaeufe","closed":false,"idBoard":"544170754ca9faba842252be","pos":16384,"subscribed":false},{"id":"544178cc3861f005f32e44b6","name":"Zu treffen","closed":false,"idBoard":"544170754ca9faba842252be","pos":65536,"subscribed":false},{"id":"5444d9f154bd69786a7a29e1","name":"Zu Besprechen","closed":false,"idBoard":"544170754ca9faba842252be","pos":131072,"subscribed":false},{"id":"54468b65f9c8194390bbded0","name":"Zu sehen","closed":false,"idBoard":"544170754ca9faba842252be","pos":147456,"subscribed":false},{"id":"545405bf68502ca272504dfd","name":"Aktivitaeten","closed":false,"idBoard":"544170754ca9faba842252be","pos":180224,"subscribed":false}]},{"id":"544d3032ffdbae1dd4214652","name":"Mein Zeug","closed":false,"idOrganization":null,"pinned":false,"lists":[{"id":"544d3032ffdbae1dd4214653","name":"Chile","closed":false,"idBoard":"544d3032ffdbae1dd4214652","pos":16384,"subscribed":false},{"id":"544d3032ffdbae1dd4214654","name":"Doing","closed":false,"idBoard":"544d3032ffdbae1dd4214652","pos":32768,"subscribed":false},{"id":"544d3032ffdbae1dd4214655","name":"Done","closed":false,"idBoard":"544d3032ffdbae1dd4214652","pos":49152,"subscribed":false},{"id":"544d303fd97af9b73aca2538","name":"Essen","closed":false,"idBoard":"544d3032ffdbae1dd4214652","pos":65536,"subscribed":false}]}]}
};

function makeRequest(urlpath, success, fail, verb) {
  var req = new XMLHttpRequest();
  if(!verb) {
    verb = "get";
  }

  if(DEBUG) {
    console.log("Debugging: faking data for urlpath "+urlpath);
    if(urlpath in DEBUG_DATA)
      success(DEBUG_DATA[urlpath]);
    else
      console.log("DEBUG_DATA not found for urlpath "+urlpath);
    fail("no such debug data", 404);
    return;
  }
  req.open(verb, 'https://api.trello.com/1/'+urlpath+'&key=e3227833b55cbe24bfedd05e5ec870dd&token='+localStorage.getItem("token"));
  req.onload = function(e) {
    if (req.readyState != 4)
        return;
    clearTimeout(timeout);
    if(req.status == 200) {
      success(JSON.parse(req.responseText));
    } else {
      console.log("Http request failed :("+ req.responseText);
      fail(req.responseText, req.status);
    }
  };
  var tries = 0;
  var timeout = setTimeout(function(){
    console.log("Timeout, aborting request");
    req.abort();
    if(tries++ <3) {
      console.log("Retrying request...");
      makeRequest(urlpath, success, fail, verb);
    } else {
      fail("Request timed out", 400);
    }
  }, 5000);
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
      //loadBoards();
    }
    return;
  }
  
  if(localStorage.getItem("token") == configuration.token) {
    console.log("Got already known token");
    return;
  }
  localStorage.setItem("token", configuration.token);
  console.log("Set token: "+configuration.token);
  loadBoards();
});

Pebble.addEventListener("appmessage",
	function(e) {
		console.log("JS: got message");
		console.log(JSON.stringify(e.payload));

    console.log("payload type:"+e.payload.type);
    switch(e.payload.type) {
      case MESSAGE_TYPE_SELECTED_LIST: 
        console.log("Got type MESSAGE_TYPE_SELECTED_LIST");
        console.log("Selected boardidx:"+e.payload.boardidx);
        globalData.activeBoard = globalData.user.boards[e.payload.boardidx];
        globalData.activeList = globalData.activeBoard.lists[e.payload.listidx];
        makeRequest('lists/'+globalData.activeList.id+'/cards?fields=id,name,checklists&checklists=all', loadedCards, loadingFailed);
        break;
      case MESSAGE_TYPE_SELECTED_CHECKLIST:
        console.log("Got type MESSAGE_TYPE_SELECTED_CHECKLIST");
        globalData.activeCard = globalData.cards[e.payload.cardidx];
        globalData.activeChecklist = globalData.activeCard.checklists[e.payload.checklistidx];
        sendActiveChecklist();
        break;
      case MESSAGE_TYPE_SELECTED_ITEM:
        console.log("Got type MESSAGE_TYPE_SELECTED_ITEM");
        var item = globalData.activeChecklist.checkItems[e.payload.itemidx];
        console.log("toggled item "+item.name+" to "+e.payload.itemstate);

        var newState = e.payload.itemstate?"complete":"incomplete";
        var checklistid = globalData.activeChecklist.id;
        function sendResult(hasFailed) {
          var msg = {};
          msg.type = MESSAGE_TYPE_ITEM_STATE_CHANGED;
          msg.checklistid = checklistid;
          msg.itemidx = e.payload.itemidx;
          var state = e.payload.itemstate;
          if(hasFailed) {
            console.log("Update has failed. Setting to old state.")
            state = (state+1)%2;
          }
          //update our cache
          item.state = state?"complete":"incomplete";
          msg.itemstate = state;
          Pebble.sendAppMessage(msg);
        }
        makeRequest("cards/"+globalData.activeCard.id+"/checklist/"+checklistid+"/checkItem/"+item.id+"/state?value="+newState, function() {
          sendResult(false);
          }, loadingFailed, "PUT");
        break;
    }
	}
);

function sendActiveChecklist() {
  var checklist = globalData.activeChecklist;
  console.log("Sending checklist "+JSON.stringify(checklist));
  var msg = {};
  msg.type = MESSAGE_TYPE_CHECKLIST;
  msg.numElements = checklist.checkItems.length;
  for(var i=0; i<msg.numElements; ++i) {
    var item = checklist.checkItems[i];
    msg[2*i] = item.name;
    msg[2*i+1] = item.state == 'incomplete'?0:1;
  }

  msg.checklistid = globalData.activeChecklist.id;

  Pebble.sendAppMessage(msg);
}

function loadBoards() {
  loadedInit = true;
  makeRequest('members/me?fields=username&boards=open&board_lists=open', loadedUser, loadingFailed);
}


function addData(msg, data) {
  msg.numElements = 0;
  var msgIndex = 0;
  for(var key in data) {
    msg.numElements += 1;
    var array = data[key];
    msg[msgIndex++] = key;
    msg[msgIndex++] = array.length;
    for(var i in array) {
      var element = array[i];
      msg[msgIndex++] = element;
    }
  }
}

function loadedCards(cards) {
  globalData.cards = cards;

  for(var i=0; i< cards.length; ++i) {
    var card = cards[i];
    card.checklists.sort(posSorting);
    for (var j=0; j< card.checklists.length; ++j) {
      card.checklists[j].checkItems.sort(posSorting);
    }
  }

  var data = {};
  for(var i=0; i<cards.length;++i) {
    data[cards[i].name] = cards[i].checklists.map(function(e){return e.name});
  }

  var msg = {};
  msg.type = MESSAGE_TYPE_CARDS;
  addData(msg, data);

  Pebble.sendAppMessage(msg);
}

function posSorting(a, b) {
  return a.pos - b.pos;
}

function loadedUser(user) {
  globalData.user = user;

  for(var i=0; i< user.boards.length; ++i) {
    var board = user.boards[i];
    board.lists.sort(posSorting);
  }

  var data = {};
  for(var i=0; i<user.boards.length;++i) {
    data[user.boards[i].name] = user.boards[i].lists.map(function(e){return e.name});
  }

  var msg = {};
  msg.type = MESSAGE_TYPE_BOARDS;
  addData(msg, data);
  
  Pebble.sendAppMessage(msg);
}

function loadingFailed(txt, code) {
  console.log("Loading failed "+code+": "+txt);
  var msg = {};
  msg.type = MESSAGE_TYPE_HTTP_FAIL;
  msg.failText = "Network failed ("+code+"):\n"+txt;
  
  Pebble.sendAppMessage(msg);
}