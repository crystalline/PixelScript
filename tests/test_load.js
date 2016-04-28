
noWindow = true;

function init() {
    print("testing JS file loading");
    load("tests/script.js");
    print("globvar="+globvar);
    if (globvar==1) print("TEST OK");
}

function update() { quit=1 }
function exit() {}
