var MESSAGE_TYPE_BOARDS = 0;
var MESSAGE_TYPE_INIT = 1;
var MESSAGE_TYPE_SELECTED_LIST = 2;
var MESSAGE_TYPE_CARDS = 3;
var MESSAGE_TYPE_SELECTED_CHECKLIST = 4;
var MESSAGE_TYPE_CHECKLIST = 5;
var MESSAGE_TYPE_SELECTED_ITEM = 6;
var MESSAGE_TYPE_ITEM_STATE_CHANGED = 7;
var MESSAGE_TYPE_HTTP_FAIL = 8;
var MESSAGE_TYPE_REFRESH_CHECKLIST = 9;
var MESSAGE_TYPE_NEW_ITEM = 11;

var loadedInit = false;
var globalData = {};


var DEBUG = false;

var DEBUG_DATA = {
};

/* Handle General API Requests
*/
function makeRequest(urlpath, success, fail, verb) {
    var req = new XMLHttpRequest();
    if(!verb) {
        verb = 'get';
    }

    if(DEBUG) {
        console.log('Debugging: faking data for urlpath '+urlpath);
        if(urlpath in DEBUG_DATA)
            success(DEBUG_DATA[urlpath]);
        else
            console.log('DEBUG_DATA not found for urlpath '+urlpath);
        fail('no such debug data', 404);
        return;
    }

    var completeURL = 'https://api.trello.com/1/'+urlpath+'&key=e3227833b55cbe24bfedd05e5ec870dd&token='+localStorage.getItem('token');
    console.log('Requesting: '+completeURL);
    req.open(verb, completeURL);
    req.onload = function() {
        if (req.readyState != 4)
            return;
        clearTimeout(timeout);
        if(req.status == 200) {
            success(JSON.parse(req.responseText));
        } else {
            console.log('Http request failed :('+ req.responseText);
            fail(req.responseText, req.status);
        }
    };
    var tries = 0;
    var timeout = setTimeout(function(){
        console.log('Timeout, aborting request');
        req.abort();
        if(tries++ <3) {
            console.log('Retrying request...');
            makeRequest(urlpath, success, fail, verb);
        } else {
            fail('Request timed out', 400);
        }
    }, 5000);
    req.send(null);
}


/* Send data to pebble reliably
*/
function sendToPebble(data) {
    Pebble.sendAppMessage(data, function() {
        console.log('pebble acked msg type '+data.type);
    }, function() {
        console.log('pebble nacked msg type '+data.type+', resending');
        // timeout to give watch some time for recovering
        setTimeout(function() {
            sendToPebble(data);
        }, 100);
    });
}

/* Initialization Function
*/
Pebble.addEventListener('ready',
    function() {
        console.log('Javascript got ready');
        var hasToken = localStorage.getItem('token')?1:0;

        if(hasToken) {
            console.log('ready: token available, loading boards');
            loadBoards();
        } else {
            var msg = {};
            msg.type = MESSAGE_TYPE_INIT;
            msg.value = hasToken;
            sendToPebble(msg);
        }
    }
);

/* ??
*/
Pebble.addEventListener('showConfiguration', function() {
    Pebble.openURL('https://trello.com/1/authorize?callback_method=fragment&scope=read,write&expiration=never&name=Pebble&key=e3227833b55cbe24bfedd05e5ec870dd&return_url=https://pebble-trello.appspot.com');
});

/* Update the auth tokens when user closes the config panel
*/
Pebble.addEventListener('webviewclosed', function (e) {
    var decoded = decodeURIComponent(e.response);
    console.log('Decoded:'+decoded);
    var configuration;
    try {
        configuration = JSON.parse(decoded);
    } catch(err) {
        if (localStorage.getItem('token') && !loadedInit) {
            console.log('token available, okay...');
        } else {
            console.log('Failed to parse:'+err);
        }
        return;
    }

    if(localStorage.getItem('token') == configuration.token) {
        console.log('Got already known token');
        return;
    }
    localStorage.setItem('token', configuration.token);
    console.log('Set token: '+configuration.token);
    loadBoards();
});

/* Parse message received from pebble watch
*/
Pebble.addEventListener('appmessage', function(e) {
    console.log('JS: got appmessage');
    console.log(JSON.stringify(e.payload));

    console.log('payload type:'+e.payload.type);
    switch(e.payload.type) {
    case MESSAGE_TYPE_SELECTED_LIST:
        console.log('Got type MESSAGE_TYPE_SELECTED_LIST');
        console.log('Selected boardidx:'+e.payload.boardidx);
        globalData.activeBoard = globalData.user.boards[e.payload.boardidx];
        globalData.activeList = globalData.activeBoard.lists[e.payload.listidx];
        makeRequest('lists/'+globalData.activeList.id+'/cards?fields=id,desc,name,checklists&checklists=all', loadedCards, loadingFailed);
        break;
    case MESSAGE_TYPE_SELECTED_CHECKLIST:
        console.log('Got type MESSAGE_TYPE_SELECTED_CHECKLIST');
        globalData.activeCard = globalData.cards[e.payload.cardidx];
        globalData.activeChecklist = globalData.activeCard.checklists[e.payload.checklistidx];
        sendActiveChecklist();
        break;
    case MESSAGE_TYPE_REFRESH_CHECKLIST:
        console.log('Got type MESSAGE_TYPE_REFRESH_CHECKLIST');
        reloadActiveChecklist();
        break;
    case MESSAGE_TYPE_NEW_ITEM:
        console.log('Got type MESSAGE_TYPE_NEW_ITEM');
        console.log('item: '+e.payload.newItem);
        var newItem = encodeURIComponent(e.payload.newItem);
        makeRequest('checklists/'+globalData.activeChecklist.id+'/checkItems?name='+newItem, function(checklist){
            reloadActiveChecklist();
        }, loadingFailed, 'POST');
        break;
    case MESSAGE_TYPE_SELECTED_ITEM:
        console.log('Got type MESSAGE_TYPE_SELECTED_ITEM');
        if (!(e.payload.itemidx in globalData.activeChecklist.checkItems)) {
            console.log('Error, unable to find item index in checklist');
            return;
        }
        var item = globalData.activeChecklist.checkItems[e.payload.itemidx];
        console.log('toggled item '+item.name+' to '+e.payload.itemstate);

        var newState = e.payload.itemstate?'complete':'incomplete';
        var checklistid = globalData.activeChecklist.id;
        function sendResult(hasFailed) {
            var msg = {};
            msg.type = MESSAGE_TYPE_ITEM_STATE_CHANGED;
            msg.checklistid = checklistid;
            msg.itemidx = e.payload.itemidx;
            var state = e.payload.itemstate;
            if(hasFailed) {
                console.log('Update has failed. Setting to old state.');
                state = (state+1)%2;
            }
            //update our cache
            item.state = state?'complete':'incomplete';
            msg.itemstate = state;
            sendToPebble(msg);
        }
        makeRequest('cards/'+globalData.activeCard.id+'/checklist/'+checklistid+'/checkItem/'+item.id+'/state?value='+newState, function() {
            sendResult(false);
        }, loadingFailed, 'PUT');
        break;
    }
});

/* WIP: Determine if a checklist exists for the selected card
*/
function checkChecklist() {
    var checklist = globalData.activeChecklist;
    console.log('Checklist of length: '+checklist.checkItems.length.toString());
    return (checklist.checkItems.length.toString() > 0);
}

/* Push the checklist to the device
*/
function sendActiveChecklist() {
    var checklist = globalData.activeChecklist;
    console.log('Sending checklist '+JSON.stringify(checklist));
    var msg = {};
    msg.type = MESSAGE_TYPE_CHECKLIST;
    msg.numElements = checklist.checkItems.length;
    // WIP
    console.log(checkChecklist());
    for(var i=0; i<msg.numElements; ++i) {
        var item = checklist.checkItems[i];
        msg[2*i] = item.name;
        msg[2*i+1] = item.state == 'incomplete'?0:1;
    }

    msg.checklistid = globalData.activeChecklist.id;
    console.log('Sent checklist ID: '+msg.toString());
    sendToPebble(msg);
}

/* ??
*/
function loadBoards() {
    loadedInit = true;
    makeRequest('members/me?fields=username&boards=open&board_lists=open&board_fields=starred,name', loadedUser, loadingFailed);
}

/* ??
*/
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

/* ??
*/
function loadedCards(cards) {
    // Attach to global data scope
    globalData.cards = cards;
    // Sort the checklist
    for(var i=0; i< cards.length; ++i) {
        var card = cards[i];
        card.checklists.sort(posSorting);
        for (var j=0; j< card.checklists.length; ++j) {
            card.checklists[j].checkItems.sort(posSorting);
        }
    }
    // Attach the card description (idx=0) and list of checklist items (idx=1+)
    var data = {};
    for(var k=0; k<cards.length; ++k) {
        // All cards have a description even if an empty string (set as index 0)
        var descArray = 'desc' in cards[k]? [cards[k].desc]:[''];
        // Append all checklist items after the description
        var cardArray = descArray.concat(cards[k].checklists.map(function(e){return e.name;}));

        // Check if both a description and checklist were added, then append to data
        if (cardArray.length >= 2) {
            data[cards[k].name] = cardArray;
        } else {
            // Otherwise skip the card
            console.log('No properties found for card: '+cards[k].name.toString());
        }
    }
    // Set message type and configure app data
    var msg = {};
    msg.type = MESSAGE_TYPE_CARDS;
    addData(msg, data);
    // Send to pebble
    sendToPebble(msg);
}

/* Sort by checklist items
*/
function posSorting(a, b) {
    return a.pos - b.pos;
}

/* Sort boards
*/
function boardSorting(a,b) {
    if(a.starred != b.starred)
        return b.starred - a.starred;
    return a.name.localeCompare(b.name);
}

/* Send the user boards to the watch
*/
function loadedUser(user) {
    console.log('loadedUser');
    console.log(user);
    globalData.user = user;

    user.boards.sort(boardSorting);
    for(var i=0; i< user.boards.length; ++i) {
        var board = user.boards[i];
        board.lists.sort(posSorting);
    }

    var data = {};
    for(var j=0; j<user.boards.length;++j) {
        data[user.boards[j].name] = user.boards[j].lists.map(function(e){return e.name;});
    }

    var msg = {};
    msg.type = MESSAGE_TYPE_BOARDS;
    addData(msg, data);

    sendToPebble(msg);
}

/* Log errors
*/
function loadingFailed(txt, code) {
    console.log('Loading failed '+code+': '+txt);
    var msg = {};
    msg.type = MESSAGE_TYPE_HTTP_FAIL;
    msg.failText = 'Network failed ('+code+'):\n'+txt;

    sendToPebble(msg);
}

/* Attempt API request to see updates of current list
*/
function reloadActiveChecklist(){
    makeRequest('checklists/'+globalData.activeChecklist.id+'?fields=none&checkItem_fields=name,pos,state', function(checklist){
        checklist.checkItems.sort(posSorting);
        globalData.activeChecklist = checklist;

        var oldChecklistRef = globalData.activeCard.checklists.filter(function(el){return el.id == checklist.id;});
        if(oldChecklistRef.length == 1)
            oldChecklistRef[0].checkItems = checklist.checkItems;
        sendActiveChecklist();
        console.log('Sent active checklist');
        console.log(oldChecklistRef);
    }, loadingFailed);
}