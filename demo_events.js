
function init() {
    print("Init");
}

function update(events) {
    print("update");
    if (events) {
        for (var i=0; i<events.length; i++) {
            print("Events: "+JSON.stringify(events[i]));
        }
    }
}

function exit() {
    print("Exit");
}

