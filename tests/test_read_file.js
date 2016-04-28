
noWindow = true;

function init() {
    print("testing JS file reading");
    var str = readFile("tests/test.txt", "utf8");
    var arr = readFile("tests/test.txt");
    print("returned "+JSON.stringify(str));
    print("returned "+JSON.stringify(arr));
    if (str == "test\n" && arr[0] == 116 && arr[4] == 10) {
        print("TEST OK");
    }
}

function update() { quit=1 }
function exit() {}
