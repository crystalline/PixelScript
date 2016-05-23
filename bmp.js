/*
 * Adapted from https://github.com/shaozilee/bmp-js/blob/master/lib/encoder.js by shaozilee
 * With Buffer shim and small modifications by Crystalline Emerald
 */

// Decoder

var console = {log: function() {
    var out = "";
    for (var i=0; i<arguments.length; i++) {
        out += arguments[i];
        out += " ";
    }
    out += "\n";
    print(out);
}};

function writeUInt32LE(uint, pos) {
    this[pos] = uint&0xFF;
    this[pos+1] = (uint&0xFF00)>>8;
    this[pos+2] = (uint&0xFF0000)>>16;
    this[pos+3] = (uint&0xFF000000)>>24;
};
function writeUInt16LE (uint, pos) {
    this[pos] = uint&0xFF;
    this[pos+1] = (uint&0xFF00)>>8;
};
function fill(byte, start, end) {
    for (var i=start; i<end; i++) {
        this[i] = byte;
    }
}
function readUInt8(pos) {
    return this[pos];
};
function readUInt16LE(pos) {
    var uint = this[pos];
    uint |= this[pos+1]<<8;
    return uint;
};
function readUInt32LE(pos) {
    var uint = this[pos];
    uint |= this[pos+1]<<8;
    uint |= this[pos+2]<<16;
    uint |= this[pos+3]<<32;
    return uint;
};

var BufferProto = {
    writeUInt32LE: writeUInt32LE,
    writeUInt16LE: writeUInt16LE,
    fill: fill,
    readUInt8: readUInt8,
    readUInt16LE: readUInt16LE,
    readUInt32LE: readUInt32LE
}

BufferProto.__proto__ = Uint8Array;

function addBufferTraits(b) {
    b.__proto__ = BufferProto;
}

function Buffer(arg) {
    if (typeof arg === "number") {
        //print("New Buffer("+arg+")");
        ret = new Uint8Array(arg);
        addBufferTraits(ret);
    } else if (arg.__proto__ == Uint8Array.prototype) {
        //print("New Buffer("+arg.length+")");
        ret = new Uint8Array(arg);
        addBufferTraits(ret);
    }
    return ret;
}

function BmpEncoder(w, h, data){
    this.buffer = data;
    this.width = w;
    this.height = h;
    this.extraBytes = this.width%4;
    this.rgbSize = this.height*(3*this.width+this.extraBytes);
    this.headerInfoSize = 40;

    this.data = [];
    /******************header***********************/
    this.flag0 = 66;
    this.flag1 = 77;
    this.reserved = 0;
    this.offset = 54;
    this.fileSize = this.rgbSize+this.offset;
    this.planes = 1;
    this.bitPP = 24;
    this.compress = 0;
    this.hr = 0;
    this.vr = 0;
    this.colors = 0;
    this.importantColors = 0;
}

BmpEncoder.prototype.encode = function() {
    var tempBuffer = new Buffer(this.offset+this.rgbSize);
    this.pos = 0;
    tempBuffer[this.pos] = this.flag0; this.pos+=1;
    tempBuffer[this.pos] = this.flag1; this.pos+=1;
    tempBuffer.writeUInt32LE(this.fileSize,this.pos);this.pos+=4;
    tempBuffer.writeUInt32LE(this.reserved,this.pos);this.pos+=4;
    tempBuffer.writeUInt32LE(this.offset,this.pos);this.pos+=4;

    tempBuffer.writeUInt32LE(this.headerInfoSize,this.pos);this.pos+=4;
    tempBuffer.writeUInt32LE(this.width,this.pos);this.pos+=4;
    tempBuffer.writeUInt32LE(this.height,this.pos);this.pos+=4;
    tempBuffer.writeUInt16LE(this.planes,this.pos);this.pos+=2;
    tempBuffer.writeUInt16LE(this.bitPP,this.pos);this.pos+=2;
    tempBuffer.writeUInt32LE(this.compress,this.pos);this.pos+=4;
    tempBuffer.writeUInt32LE(this.rgbSize,this.pos);this.pos+=4;
    tempBuffer.writeUInt32LE(this.hr,this.pos);this.pos+=4;
    tempBuffer.writeUInt32LE(this.vr,this.pos);this.pos+=4;
    tempBuffer.writeUInt32LE(this.colors,this.pos);this.pos+=4;
    tempBuffer.writeUInt32LE(this.importantColors,this.pos);this.pos+=4;

    var i=0;
    var rowBytes = 3*this.width+this.extraBytes;

    for (var y = this.height - 1; y >= 0; y--){
        for (var x = 0; x < this.width; x++){
            var p = this.pos+y*rowBytes+x*3;
            tempBuffer[p+2]= this.buffer[i++];//r
            tempBuffer[p+1] = this.buffer[i++];//g
            tempBuffer[p]  = this.buffer[i++];//b
            i++;
        }
        if(this.extraBytes>0){
            var fillOffset = this.pos+y*rowBytes+this.width*3;
            tempBuffer.fill(0,fillOffset,fillOffset+this.extraBytes);   
        }
    }

    return tempBuffer;
};

function encodeBmp(w, h, imgDataData) {
    var encoder = new BmpEncoder(w, h, imgDataData);
    return encoder.encode();
}

// Decoder

function BmpDecoder(buffer) {
  this.pos = 0;
  this.buffer = buffer;
  //this.flag = this.buffer.toString("utf-8", 0, this.pos += 2);
  this.flag0 = this.buffer[0];
  this.flag1 = this.buffer[1];
  this.pos += 2;
  if (this.flag0 != 66 || this.flag1 != 77) throw new Error("Invalid BMP File");
  this.parseHeader();
  this.parseBGR();
}

BmpDecoder.prototype.parseHeader = function() {
  this.fileSize = this.buffer.readUInt32LE(this.pos);
  this.pos += 4;
  this.reserved = this.buffer.readUInt32LE(this.pos);
  this.pos += 4;
  this.offset = this.buffer.readUInt32LE(this.pos);
  this.pos += 4;
  this.headerSize = this.buffer.readUInt32LE(this.pos);
  this.pos += 4;
  this.width = this.buffer.readUInt32LE(this.pos);
  this.pos += 4;
  this.height = this.buffer.readUInt32LE(this.pos);
  this.pos += 4;
  this.planes = this.buffer.readUInt16LE(this.pos);
  this.pos += 2;
  this.bitPP = this.buffer.readUInt16LE(this.pos);
  this.pos += 2;
  this.compress = this.buffer.readUInt32LE(this.pos);
  this.pos += 4;
  this.rawSize = this.buffer.readUInt32LE(this.pos);
  this.pos += 4;
  this.hr = this.buffer.readUInt32LE(this.pos);
  this.pos += 4;
  this.vr = this.buffer.readUInt32LE(this.pos);
  this.pos += 4;
  this.colors = this.buffer.readUInt32LE(this.pos);
  this.pos += 4;
  this.importantColors = this.buffer.readUInt32LE(this.pos);
  this.pos += 4;

  if (this.bitPP < 24) {
    var len = this.colors === 0 ? 1 << this.bitPP : this.colors;
    this.palette = new Array(len);
    for (var i = 0; i < len; i++) {
      var blue = this.buffer.readUInt8(this.pos++);
      var green = this.buffer.readUInt8(this.pos++);
      var red = this.buffer.readUInt8(this.pos++);
      var quad = this.buffer.readUInt8(this.pos++);
      this.palette[i] = {
        red: red,
        green: green,
        blue: blue,
        quad: quad
      };
    }
  }

}

BmpDecoder.prototype.parseBGR = function() {
  this.pos = this.offset;
  try {
    var bitn = "bit" + this.bitPP;
    var len = this.width * this.height * 4;
    this.data = new Buffer(len);

    this[bitn]();
  } catch (e) {
    console.log("bit decode error:" + e);
  }

};

BmpDecoder.prototype.bit1 = function() {
  var xlen = Math.ceil(this.width / 8);
  var mode = xlen%4;
  for (var y = this.height - 1; y >= 0; y--) {
    for (var x = 0; x < xlen; x++) {
      var b = this.buffer.readUInt8(this.pos++);
      var location = y * this.width * 4 + x*8*4;
      for (var i = 0; i < 8; i++) {
        if(x*8+i<this.width){
          var rgb = this.palette[((b>>(7-i))&0x1)];
          this.data[location+i*4] = rgb.blue;
          this.data[location+i*4 + 1] = rgb.green;
          this.data[location+i*4 + 2] = rgb.red;
          this.data[location+i*4 + 3] = 0xFF;
        }else{
          break;
        }
      }
    }

    if (mode != 0){
      this.pos+=(4 - mode);
    }
  }
};

BmpDecoder.prototype.bit4 = function() {
  var xlen = Math.ceil(this.width/2);
  var mode = xlen%4;
  for (var y = this.height - 1; y >= 0; y--) {
    for (var x = 0; x < xlen; x++) {
      var b = this.buffer.readUInt8(this.pos++);
      var location = y * this.width * 4 + x*2*4;

      var before = b>>4;
      var after = b&0x0F;

      var rgb = this.palette[before];
      this.data[location] = rgb.blue;
      this.data[location + 1] = rgb.green;
      this.data[location + 2] = rgb.red;
      this.data[location + 3] = 0xFF;

      if(x*2+1>=this.width)break;

      rgb = this.palette[after];
      this.data[location+4] = rgb.blue;
      this.data[location+4 + 1] = rgb.green;
      this.data[location+4 + 2] = rgb.red;
      this.data[location+4 + 3] = 0xFF;
    }

    if (mode != 0){
      this.pos+=(4 - mode);
    }
  }

};

BmpDecoder.prototype.bit8 = function() {
  var mode = this.width%4;
  for (var y = this.height - 1; y >= 0; y--) {
    for (var x = 0; x < this.width; x++) {
      var b = this.buffer.readUInt8(this.pos++);
      var location = y * this.width * 4 + x*4;
      if(b < this.palette.length) {
        var rgb = this.palette[b];
        this.data[location] = rgb.blue;
        this.data[location + 1] = rgb.green;
        this.data[location + 2] = rgb.red;
        this.data[location + 3] = 0xFF;
      } else {
        this.data[location] = 0xFF;
        this.data[location + 1] = 0xFF;
        this.data[location + 2] = 0xFF;
        this.data[location + 3] = 0xFF;
      }
    }
    if (mode != 0){
      this.pos+=(4 - mode);
    }
  }
};

BmpDecoder.prototype.bit24 = function() {
  //when height > 0
  for (var y = this.height - 1; y >= 0; y--) {
    for (var x = 0; x < this.width; x++) {
      var blue = this.buffer.readUInt8(this.pos++);
      var green = this.buffer.readUInt8(this.pos++);
      var red = this.buffer.readUInt8(this.pos++);
      var location = y * this.width * 4 + x * 4;
      this.data[location] = red;
      this.data[location + 1] = green;
      this.data[location + 2] = blue;
      this.data[location + 3] = 0xFF;
    }
    //skip extra bytes
    this.pos += (this.width % 4);
  }

};

/**
 * add 32bit decode func
 * @author soubok
 */
BmpDecoder.prototype.bit32 = function() {
  //when height > 0
  for (var y = this.height - 1; y >= 0; y--) {
    for (var x = 0; x < this.width; x++) {
      var blue = this.buffer.readUInt8(this.pos++);
      var green = this.buffer.readUInt8(this.pos++);
      var red = this.buffer.readUInt8(this.pos++);
      var alpha = this.buffer.readUInt8(this.pos++);
      var location = y * this.width * 4 + x * 4;
      this.data[location] = red;
      this.data[location + 1] = green;
      this.data[location + 2] = blue;
      this.data[location + 3] = alpha;
    }
    //skip extra bytes
    this.pos += (this.width % 4);
  }

};

BmpDecoder.prototype.getData = function() {
  return this.data;
};

function decodeBMP(bmpData) {
    var arg = bmpData;
    if (bmpData.__proto__ == Uint8Array.prototype) {
        arg = new Buffer(bmpData)
    }
    var decoder = new BmpDecoder(arg);
    return {
        data: decoder.getData(),
        width: decoder.width,
        height: decoder.height
    };
}
