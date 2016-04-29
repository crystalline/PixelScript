
noWindow = true;

function cmpArr(A,B) {
    if (!A || !B || (A.length !== B.length)) return;
    for (var i=0; i<A.length; i++) { if (A[i] !== B[i]) return false; }
    return true;
}

function init() {
    print("testing JS file writing");
    var data = new Uint8Array([0xBB,0xEE,0xEE,0xFF]);
    var str = "test";
    
    var failure = false;
    function fail() { print("TEST FAILED"); failure = true; };
    
    writeFile("tests/test_str.txt", str);
    if (str == readFile("tests/test_str.txt", "utf8")) {
        print("writeFile @ String : [OK]");
    } else fail();
    
    writeFile("tests/test_arr.bin", data);
    var rdata;
    if ((rdata = readFile("tests/test_arr.bin")) && typeof rdata === typeof data && cmpArr(rdata, data)) {
        print("writeFile @ Uint8Array : [OK]");
    } else fail();
    print("Rdata: type="+typeof rdata+" length="+rdata.length);
    
    print("JS readFile utf8: "+readFile("tests/test_str.txt", "utf8").length);
    print("JS readFile bin: "+readFile("tests/test_arr.bin").length);
    
    if (!failure) {
        print("TESTS PASSED!");
    } else {
        print("TESTS FAILED!");
    }
}

function update() { quit=1 }
function exit() {}
