//A very simple 2d game engine built with SDL2 and V8

#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdint.h>
#include <math.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "v8/include/libplatform/libplatform.h"
#include "v8/include/v8.h"

#define COMPILE_BLOBS 1
#define COMPILE_LOGO 1

#ifdef COMPILE_BLOBS
#include "natives_blob.c"
#include "snapshot_blob.c"
#endif

#ifdef COMPILE_LOGO
#include "logo.c"
#endif

#define LOGO_W 32
#define LOGO_H 32
#define LOGO_BPP 3

#define GLOBAL_QUIT "quit"
#define GLOBAL_PAUSE "pause"
#define GLOBAL_TITLE "windowTitle"
#define GLOBAL_NOWINDOW "noWindow"
#define GLOBAL_HIDECURSOR "hideCursor"
#define GLOBAL_SW "screenWidth"
#define GLOBAL_SH "screenHeight"
#define GLOBAL_PRINT "print"
#define GLOBAL_LOAD "load"
#define GLOBAL_READFILE "readFile"
#define GLOBAL_WRITEFILE "writeFile"
#define GLOBAL_APPENDFILE "appendFile"
#define GLOBAL_INIT "init"
#define GLOBAL_UPDATE "update"
#define GLOBAL_EXIT "exit"
#define GLOBAL_FRAMEBUFFER "screen"

#define LOG_PREFIX "LOG: "

#define DEBUG 1

#define PRINT_BYTES(ptr, size) printf("bytes: "); for (int i=0; i<size; i++) { printf("%hhx ", ((unsigned char*)ptr)[i]); } puts("");

enum EvTypes {
    EV_QUIT, EV_KEY_UP, EV_KEY_DOWN, EV_MOUSE_UP, EV_MOUSE_DOWN, EV_MOUSE_MOVE
};

using namespace v8;

int screenWidth, screenHeight;
bool pause = 0;
bool quit = 0;
double fps = 50.0;
bool appHideCursor = false;
bool prevHideCursor = false;

class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
    public:
    virtual void* Allocate(size_t length) {
        void* data = AllocateUninitialized(length);
        return data == NULL ? data : memset(data, 0, length);
    }
    virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
    virtual void Free(void* data, size_t) { free(data); }
};

void Log(const char* event) { printf(LOG_PREFIX "%s\n", event); }
void Error(const char* desc) { printf("Error: %s\n", desc); }
void Error(const char* desc, const char* value) { printf("Error: %s %s\n", desc, value); }

bool FileExists(const char* name) {
    if (FILE *file = fopen(name, "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}

char* FileRead(const char* name, size_t* retSize) {
    FILE* file = fopen(name, "rb");
    if (file == NULL) return NULL;

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    *retSize = size;
    rewind(file);

    char* chars = new char[size + 1];
    chars[size] = '\0';
  
    for (size_t i = 0; i < size;) {
        i += fread(&chars[i], 1, size - i, file);
        if (ferror(file)) {
            fclose(file);
            return NULL;
        }
    }
      
    fclose(file);
    return chars;;
}

char* FileRead(const char* name) {
    size_t size;
    return FileRead(name, &size);
}

bool FileWrite(const char* name, char* data, size_t size) {
    
    printf("FileWrite: %s, size=%zu ", name, size); PRINT_BYTES(data, size);
    
    FILE* file = fopen(name, "wb");
    if (file == NULL) return false;
    fwrite(data, 1, size, file);
    fclose(file);
    return true;
}

bool FileAppend(const char* name, char* data, size_t size) {
    
    printf("FileAppend: %s, size=%zu ", name, size); PRINT_BYTES(data, size);
    
    FILE* file = fopen(name, "ab");
    if (file == NULL) return false;
    fwrite(data, 1, size, file);
    fclose(file);
    return true;
}

bool ExecuteScript(Isolate* isolate, Local<String> script) {
    HandleScope handle_scope(isolate);
    
    // We're just about to compile the script; set up an error handler to
    // catch any exceptions the script might throw.
    TryCatch try_catch(isolate);

    Local<Context> context(isolate->GetCurrentContext());

    // Compile the script and check for errors.
    Local<Script> compiled_script;
    
    if (!Script::Compile(context, script).ToLocal(&compiled_script)) {
        String::Utf8Value error(try_catch.Exception());
        Log(*error);
        // The script failed to compile; bail out.
        return false;
    }

    // Run the script!
    Local<Value> result;
    if (!compiled_script->Run(context).ToLocal(&result)) {
        // The TryCatch above is still in effect and will have caught the error.
        String::Utf8Value error(try_catch.Exception());
        Log(*error);
        // Running the script failed; bail out.
        return false;
    }
    
    /*
    String::Utf8Value utf8(result);
    printf("%s\n", *utf8);
    */
    
    return true;
}

bool ExecuteScript(Isolate* isolate, const char* s) {
    //Create a string containing the JavaScript source code.
    Local<String> source =
        String::NewFromUtf8(isolate, s, NewStringType::kNormal).ToLocalChecked();
    
    return ExecuteScript(isolate, source);
}

bool CheckFnExists(Isolate* isolate, const char* fnName) {
    Local<Context> context(isolate->GetCurrentContext());
    
    Local<String> jsFnName =
        String::NewFromUtf8(isolate, fnName, NewStringType::kNormal).ToLocalChecked();
    
    Local<Value> fnVal;
    
    return context->Global()->Get(context, jsFnName).ToLocal(&fnVal) && fnVal->IsFunction();
}

Local<Function> GetFn(Isolate* isolate, const char* fnName) {
    
    Local<Context> context(isolate->GetCurrentContext());
    
    EscapableHandleScope handle_scope(isolate);
    
    // The script compiled and ran correctly.  Now we fetch out the
    // Process function from the global object.
    Local<String> process_name =
        String::NewFromUtf8(isolate, fnName, NewStringType::kNormal)
        .ToLocalChecked();
    
    Local<Value> process_val;
    
    // If there is no Process function, or if it is not a function,
    // bail out
    bool OK = context->Global()->Get(context, process_name).ToLocal(&process_val);
    Local<Function> process_fun = Local<Function>::Cast(process_val);
    
    return handle_scope.Escape(process_fun);
}

double GetNumber(Isolate* isolate, const char* globName, double defaultValue) {
    Local<Context> context(isolate->GetCurrentContext());
    HandleScope handle_scope(isolate);
    Local<String> name = String::NewFromUtf8(isolate, globName, NewStringType::kNormal).ToLocalChecked();
    
    Local<Value> val;
    bool OK = context->Global()->Get(context, name).ToLocal(&val) && val->IsNumber();
    
    if (OK) {
        return val->NumberValue();
    } else {
        return defaultValue;
    }
}

const char* GetString(Isolate* isolate, const char* globName, const char* defaultValue) {
    Local<Context> context(isolate->GetCurrentContext());
    HandleScope handle_scope(isolate);
    Local<String> name = String::NewFromUtf8(isolate, globName, NewStringType::kNormal).ToLocalChecked();
    
    Local<Value> val;
    bool OK = context->Global()->Get(context, name).ToLocal(&val) && val->IsString();
    
    if (OK) {
        String::Utf8Value* ret = new String::Utf8Value(val);
        return **ret;
    } else {
        return defaultValue;
    }
}

bool GetBoolean(Isolate* isolate, const char* globName) {
    Local<Context> context(isolate->GetCurrentContext());
    HandleScope handle_scope(isolate);
    Local<String> name = String::NewFromUtf8(isolate, globName, NewStringType::kNormal).ToLocalChecked();
    
    Local<Value> val;
    bool OK = context->Global()->Get(context, name).ToLocal(&val);
    
    if (OK) {
        return val->BooleanValue();
    } else {
        return false;
    }
}

void makeGlobalByteArray(Isolate* isolate, const char* globName, size_t size, void* data) {
    Local<Context> context(isolate->GetCurrentContext());
    EscapableHandleScope handle_scope(isolate);
    Local<ArrayBuffer> buffer = ArrayBuffer::New(isolate, data, size);
    Local<Uint8Array> jsbuffer = Uint8Array::New(buffer, 0, size);
    Local<String> name = String::NewFromUtf8(isolate, globName, NewStringType::kNormal).ToLocalChecked();
    Maybe<bool> OK = context->Global()->Set(context, name, jsbuffer);
}

void clearScreen(uint32_t* pixels) {
    int i;
    size_t N = screenWidth*screenHeight*4;
    memset(pixels, 0, N);
}

void handleMouseDown(SDL_MouseButtonEvent* event) {
    int x, y;    
    x = event->x;
    y = event->y;
}

void handleMouseUp(SDL_MouseButtonEvent* event) {
    int x, y;    
    x = event->x;
    y = event->y;
}

void handleMouseMove(SDL_MouseMotionEvent* event) {
    int x, y;    
    x = event->x;
    y = event->y;
}

void handleKeyDown(SDL_KeyboardEvent* event) {
    SDL_Keysym key = event->keysym;
    int Scode = key.scancode;
    if (Scode == SDL_SCANCODE_ESCAPE) { quit = 1; }
}

void handleKeyUp(SDL_KeyboardEvent* event) {
    SDL_Keysym key = event->keysym;
    int Scode = key.scancode;
}

void handleQuit() {
    quit = 1;
}

typedef struct logo {
    unsigned int width;
    unsigned int height;
    unsigned int bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
    unsigned char pixel_data[LOGO_W * LOGO_H * LOGO_BPP + 1];
} logo;

void SetIcon(SDL_Window* win, logo* logo) {
    SDL_Surface *icon;
    //printf("Loading logo, w=%d, h=%d, bpp=%d\n", logo->width, logo->height, logo->bytes_per_pixel);
    uint32_t rmask,gmask,amask,bmask;
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        int shift = (logo->bytes_per_pixel == 3) ? 8 : 0;
        rmask = 0xff000000 >> shift;
        gmask = 0x00ff0000 >> shift;
        bmask = 0x0000ff00 >> shift;
        amask = 0x000000ff >> shift;
    #else // little endian, like x86
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = (logo->bytes_per_pixel == 3) ? 0 : 0xff000000;
    #endif
    icon = SDL_CreateRGBSurfaceFrom((void*)logo->pixel_data, logo->width,
        logo->height, logo->bytes_per_pixel*8, logo->bytes_per_pixel*logo->width,
        rmask, gmask, bmask, amask);
    SDL_SetWindowIcon(win, icon);
    SDL_FreeSurface(icon);
}

bool ExecGlobFn(Isolate* isolate, const char* fn_name, bool checkExists=false) {
    Local<Context> context(isolate->GetCurrentContext());
    TryCatch try_catch(isolate);
    
    if (checkExists) {
        if (!CheckFnExists(isolate, "process")) {
            Error("no function found:", fn_name);
            return false;
        }
    }
    // Get function
    Local<Function> fn = GetFn(isolate, fn_name);
    // Invoke the process function, giving the global object as 'this' and 0 arguments
    const unsigned argc = 0;
    Local<Value> argv[argc] = {  };

    Local<Value> result;
    
    if (!fn->Call(context, context->Global(), argc, argv).ToLocal(&result)) {
        String::Utf8Value error(try_catch.Exception());
        printf("Exception in function \"%s\": %s\n", fn_name, *error);
        return false;
    } else {
        return true;
    }
}

bool ExecGlobFnArg(Isolate* isolate, const char* fn_name, Local<Value> arg, bool checkExists=false) {
    Local<Context> context(isolate->GetCurrentContext());
    TryCatch try_catch(isolate);
    
    if (checkExists) {
        if (!CheckFnExists(isolate, "process")) {
            Error("no function found:", fn_name);
            return false;
        }
    }
    // Get function
    Local<Function> fn = GetFn(isolate, fn_name);
    // Invoke the process function, giving the global object as 'this'
    const unsigned argc = 1;
    Local<Value> argv[argc] = { arg };

    Local<Value> result;
    
    if (!fn->Call(context, context->Global(), argc, argv).ToLocal(&result)) {
        String::Utf8Value error(try_catch.Exception());
        printf("Exception in function \"%s\": %s\n", fn_name, *error);
        return false;
    } else {
        return true;
    }
}

static void LogCallback(const FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() < 1) return;
    HandleScope scope(args.GetIsolate());
    Local<Value> arg = args[0];
    String::Utf8Value value(arg);
    
    Log(*value);
}

static void LoadCallback(const FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() < 1) return;
    Isolate* isolate = args.GetIsolate();
    HandleScope scope(isolate);
    Local<Value> arg = args[0];
    String::Utf8Value value(arg);
    
    char* source = NULL;
    
    if (FileExists(*value) && (source = FileRead(*value))) {
        if (!ExecuteScript(isolate, source)) {
            Error("couldn't execute javascript file", *value);
        }
    } else {
        Error("cannot load javascript file", *value);
    }
}

static void ReadFileCallback(const FunctionCallbackInfo<v8::Value>& args) {
    Isolate* isolate = args.GetIsolate();
    EscapableHandleScope handle_scope(isolate);
    
    if (args.Length() < 1) {
        args.GetReturnValue().Set(handle_scope.Escape(Undefined(isolate)));
        return;
    }
    
    Local<Value> arg0 = args[0];
    String::Utf8Value file_path(arg0);
    
    bool encodingBinary = true;
    
    if (args.Length() >= 2) {
        Local<Value> arg1 = args[1];
        String::Utf8Value encoding(arg1);
        if (strcmp(*encoding, "utf8") == 0) {
            encodingBinary = false;
        }
    }
    
    size_t size;
    char* data = FileRead(*file_path, &size);
    
    printf("ReadFile path=%s size=%zu data \n", *file_path, size); PRINT_BYTES(data, size);
    
    if (data == NULL) {
        args.GetReturnValue().Set(handle_scope.Escape(Undefined(isolate)));
        return;
    }
    
    Local<Value> ret;
    
    if (encodingBinary) {
        Local<ArrayBuffer> buffer = ArrayBuffer::New(isolate, size);
        ArrayBuffer::Contents bcont = buffer->GetContents();
        void* bufPtr = bcont.Data();
        memcpy(bufPtr, data, size);
        
        Local<Uint8Array> aret = Uint8Array::New(buffer, 0, size);
        args.GetReturnValue().Set(handle_scope.Escape(aret));
    } else {
        Local<String> sret;
        bool OK = String::NewFromUtf8(isolate, data, NewStringType::kNormal).ToLocal(&sret);
        args.GetReturnValue().Set(handle_scope.Escape(sret));
    }
    
    delete data;
}

static void _WriteFile(const FunctionCallbackInfo<v8::Value>& args, bool append=false) {
    Isolate* isolate = args.GetIsolate();
    EscapableHandleScope handle_scope(isolate);
    
    const char* fnName = append ? "appendFile" : "writeFile";
    
    if (args.Length() < 2) {
        printf("Error: Not enough arguments to %s(file_path, data)\n", fnName);
        args.GetReturnValue().Set(handle_scope.Escape(Undefined(isolate)));
        return;
    }
    
    Local<Value> arg0 = args[0];
    //Persistent<Value> _arg0(isolate, arg0);
    String::Utf8Value file_path(arg0);
    
    Local<Value> arg1 = args[1];
    //Persistent<Value> _arg1(isolate, arg1);
    
    bool encodingBinary = false;
        
    if (arg1->IsString()) {
        encodingBinary = false;
    } else if (arg1->IsUint8Array()) {
        encodingBinary = true;
    } else {
        printf("Error: Illegal second argument to %s(file_path, data). It should be either string or Uint8Array\n", fnName);
    }
    
    char* dataPtr;
    size_t dataLength;
    //Persistent<Uint8Array> _dataArr;
    
    if (encodingBinary) {
        Local<Uint8Array> dataArr = arg1.As<Uint8Array>();
        //_dataArr.Reset(isolate, dataArr);
        
        ArrayBuffer::Contents bcont = dataArr->Buffer()->GetContents();
        const size_t dataOffset = dataArr->ByteOffset();
        dataLength = dataArr->ByteLength();
        dataPtr = static_cast<char*>(bcont.Data()) + dataOffset;
        
        if (dataPtr == NULL) {
            Error("Error: %s cannot get data pointer from the Uint8Array argument\n", fnName);
            return;
        }
        
        printf("_WriteFile @ bin "); PRINT_BYTES(dataPtr, dataLength);
        
        if (append) {
            FileAppend(*file_path, dataPtr, dataLength);
        } else {
            FileWrite(*file_path, dataPtr, dataLength);
        }
    } else {
        String::Utf8Value string_data(Local<Value>::New(isolate, arg1));
        dataPtr = *string_data;
        dataLength = string_data.length();
        
        printf("_WriteFile @ utf8 "); PRINT_BYTES(dataPtr, dataLength);
        
        if (append) {
            FileAppend(*file_path, dataPtr, dataLength);
        } else {
            FileWrite(*file_path, dataPtr, dataLength);
        }
    }
    
    //_arg0.Reset();
    //_arg1.Reset();
    //_dataArr.Reset();
}

static void WriteFileCallback(const FunctionCallbackInfo<v8::Value>& args) {
    _WriteFile(args, false);
}

static void AppendFileCallback(const FunctionCallbackInfo<v8::Value>& args) {
    _WriteFile(args, true);
}

#define ENGINE_API(js_name, cpp_name) \
    (context->Global()->Set(String::NewFromUtf8(isolate, js_name, NewStringType::kNormal).ToLocalChecked(), \
                            FunctionTemplate::New(isolate, cpp_name)->GetFunction()));

int main(int argc, char* argv[]) {
    
    char* sourcePath = NULL;
    
    if (argc < 2 || !FileExists(argv[1])) {
        printf("Valid usage: ./pixelscript file.js\n");
        printf("No source file given, trying to load \"app.js\"\n");
        if (!FileExists("app.js")) {
            printf("No \"app.js\" found, exiting...\n");
            return -1;
        }
        sourcePath = (char*) "app.js";
    } else {
        sourcePath = argv[1];
    }
    
    const char* source = FileRead(sourcePath);
    
    // Initialize V8.
    V8::InitializeICU();
    
#ifdef COMPILE_BLOBS
    static StartupData natives;
    natives.data = (const char*) v8_natives_blob;
    natives.raw_size = sizeof(v8_natives_blob);
    static StartupData snapshot;
    snapshot.data = (const char*) v8_snapshot_blob;
    snapshot.raw_size = sizeof(v8_snapshot_blob);
    V8::SetNativesDataBlob(&natives);
    V8::SetSnapshotDataBlob(&snapshot);
#else
    V8::InitializeExternalStartupData(argv[0]);
#endif
    
    Platform* platform = platform::CreateDefaultPlatform();
    V8::InitializePlatform(platform);
    V8::Initialize();

    // Create a new Isolate and make it the current one.
    ArrayBufferAllocator allocator;
    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = &allocator;
    Isolate* isolate = Isolate::New(create_params);
    
    {
        Isolate::Scope isolate_scope(isolate);
        // Create a stack-allocated handle scope.
        HandleScope handle_scope(isolate);
        // Create a new context.
        Local<Context> context = Context::New(isolate, NULL);
        // Enter the context
        Context::Scope context_scope(context);
        
        // Set globals
        ENGINE_API(GLOBAL_PRINT, LogCallback)
        ENGINE_API(GLOBAL_LOAD, LoadCallback)
        ENGINE_API(GLOBAL_READFILE, ReadFileCallback)
        ENGINE_API(GLOBAL_WRITEFILE, WriteFileCallback)
        ENGINE_API(GLOBAL_APPENDFILE, AppendFileCallback)
        
        ExecuteScript(isolate, source);
        
        bool noFnFound = false;
        
        if (!CheckFnExists(isolate, GLOBAL_INIT)) {
            noFnFound = true; printf("No '" GLOBAL_INIT "' function found. '" GLOBAL_INIT "' is required by the engine, please create it\n");
        }
        if (!CheckFnExists(isolate, GLOBAL_UPDATE)) {
            noFnFound = true; printf("No '" GLOBAL_UPDATE "' function found. '" GLOBAL_UPDATE "' is required by the engine, please create it\n");
        }
        if (!CheckFnExists(isolate, GLOBAL_EXIT)) {
            noFnFound = true; printf("No '" GLOBAL_EXIT "' function found. '" GLOBAL_EXIT "' is required by the engine, please create it\n");
        }
        
        if (noFnFound) { printf("Exiting\n"); return -1; }
        
        ExecGlobFn(isolate, GLOBAL_INIT);
        
        bool noGraphics = false;
        
        //Check if application wants to run headless
        bool noWindow = GetBoolean(isolate, GLOBAL_NOWINDOW);
        
        quit = 0;
        
        SDL_Window *win = NULL;
        SDL_Renderer *renderer = NULL;
        SDL_Texture *screen = NULL;
        uint32_t* pixels;
        
        //If it does then enter simple headless mainloop
        if (noWindow) {
            
            while (!quit) {
                ExecGlobFn(isolate, GLOBAL_UPDATE);
                quit = quit || GetBoolean(isolate, GLOBAL_QUIT);
            }
            
            ExecGlobFn(isolate, GLOBAL_EXIT);
            
        } else {
            //Default windowed mode. Try to init SDL2 window and enter the Main Event Loop
            
            screenWidth = (int) GetNumber(isolate, GLOBAL_SW, 640);
            screenHeight = (int) GetNumber(isolate, GLOBAL_SH, 480);
            const char* windowTitle = GetString(isolate, GLOBAL_TITLE, "SDL2 V8 Application");
            
            SDL_Init(SDL_INIT_VIDEO);
            
            win = SDL_CreateWindow(windowTitle, 0, 0, screenWidth, screenHeight, 0);
            
#ifdef COMPILE_LOGO
            SetIcon(win, (logo*) &js_logo);
#endif
            renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
            screen = SDL_CreateTexture(renderer,
                                       SDL_PIXELFORMAT_ABGR8888,//SDL_PIXELFORMAT_RGBA8888, //SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       screenWidth, screenHeight);
            
            size_t fbSize = sizeof(uint32_t)*screenWidth*screenHeight;
            pixels = (uint32_t*) malloc(fbSize);
            uint32_t beforeT, afterT;
            
            makeGlobalByteArray(isolate, GLOBAL_FRAMEBUFFER, fbSize, pixels);
            
            if (noGraphics) {
                printf("ERROR: Couldn't init graphics subsystem, perhaps you forgot to set some global variables\nExiting\n");
                return -1;
            }
            
            // Mainloop
            while (!quit) {
                        
                beforeT = SDL_GetTicks();

                SDL_Event e;
                
                Local<Array> jsEventArr;
                bool evArrayCreated = false;
                size_t evCount = 0;
                
                while (SDL_PollEvent(&e)) {
                    
                    if (!evArrayCreated) { jsEventArr = Array::New(isolate, 1); evArrayCreated = true; }
                    
                    if (e.type == SDL_QUIT) {
                        quit = 1;
                        Local<Object> event = Object::New(isolate);
                        event->Set(String::NewFromUtf8(isolate, "type"), Integer::New(isolate, SDL_QUIT));
                        jsEventArr->Set(evCount++, event);
                    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                        Local<Object> event = Object::New(isolate);
                        event->Set(String::NewFromUtf8(isolate, "type"), Integer::New(isolate, SDL_MOUSEBUTTONDOWN));
                        event->Set(String::NewFromUtf8(isolate, "x"), Integer::New(isolate, e.button.x));
                        event->Set(String::NewFromUtf8(isolate, "y"), Integer::New(isolate, e.button.y));
                        event->Set(String::NewFromUtf8(isolate, "button"), Integer::New(isolate, e.button.button));
                        jsEventArr->Set(evCount++, event);
                    } else if (e.type == SDL_MOUSEBUTTONUP) {
                        Local<Object> event = Object::New(isolate);
                        event->Set(String::NewFromUtf8(isolate, "type"), Integer::New(isolate, SDL_MOUSEBUTTONUP));
                        event->Set(String::NewFromUtf8(isolate, "x"), Integer::New(isolate, e.button.x));
                        event->Set(String::NewFromUtf8(isolate, "y"), Integer::New(isolate, e.button.y));
                        event->Set(String::NewFromUtf8(isolate, "button"), Integer::New(isolate, e.button.button));
                        jsEventArr->Set(evCount++, event);
                    } else  if (e.type == SDL_MOUSEMOTION) {
                        Local<Object> event = Object::New(isolate);
                        event->Set(String::NewFromUtf8(isolate, "type"), Integer::New(isolate, SDL_MOUSEMOTION));
                        event->Set(String::NewFromUtf8(isolate, "x"), Integer::New(isolate, e.motion.x));
                        event->Set(String::NewFromUtf8(isolate, "y"), Integer::New(isolate, e.motion.y));
                        jsEventArr->Set(evCount++, event);
                    } else if (e.type == SDL_KEYDOWN) {
                        SDL_Keysym key = e.key.keysym;
                        int Scode = key.scancode;
                        if (Scode == SDL_SCANCODE_ESCAPE) { quit = 1; }
                        Local<Object> event = Object::New(isolate);
                        event->Set(String::NewFromUtf8(isolate, "type"), Integer::New(isolate, SDL_KEYDOWN));
                        event->Set(String::NewFromUtf8(isolate, "code"), Integer::New(isolate, Scode));
                        jsEventArr->Set(evCount++, event);
                    } else if (e.type == SDL_KEYUP) {
                        SDL_Keysym key = e.key.keysym;
                        int Scode = key.scancode;
                        Local<Object> event = Object::New(isolate);
                        event->Set(String::NewFromUtf8(isolate, "type"), Integer::New(isolate, SDL_KEYUP));
                        event->Set(String::NewFromUtf8(isolate, "code"), Integer::New(isolate, Scode));
                        jsEventArr->Set(evCount++, event);
                    }
                }
                
                if (!pause) {
                    
                    clearScreen(pixels);
                    
                    //Call update either with event array or with undefined
                    if (evArrayCreated) {
                        ExecGlobFnArg(isolate, GLOBAL_UPDATE, jsEventArr);
                    } else {
                        ExecGlobFn(isolate, GLOBAL_UPDATE);
                    }
                    
                    quit = quit || GetBoolean(isolate, GLOBAL_QUIT);
                    
                    //Update cursor show/hide state on each frame
                    appHideCursor = GetBoolean(isolate, GLOBAL_HIDECURSOR);
                    if (appHideCursor && !prevHideCursor) { SDL_ShowCursor(0); }
                    if (!appHideCursor && prevHideCursor) { SDL_ShowCursor(1); }
                    prevHideCursor = appHideCursor;
                    
                    SDL_UpdateTexture(screen, NULL, pixels, screenWidth * sizeof(uint32_t));
                    
                    SDL_RenderClear(renderer);
                    SDL_RenderCopy(renderer, screen, NULL, NULL);
                    SDL_RenderPresent(renderer);
                    
                    afterT = SDL_GetTicks();
                    
                    if (afterT - beforeT < 1000.0/fps) {
                        SDL_Delay(1000.0/fps - (afterT - beforeT));
                        //printf("delay: %f\n", 1000.0/fps - (afterT - beforeT));
                    }
                } else {
                    SDL_Delay((int)1000.0/fps);
                }
            }
            
            ExecGlobFn(isolate, GLOBAL_EXIT);
                        
            SDL_DestroyTexture(screen);
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(win);
            SDL_Quit();
        }
    }
    
    // puts("Exit from V8");
    isolate->Dispose();
    V8::Dispose();
    V8::ShutdownPlatform();
    delete platform;
    
    return 0;
}
