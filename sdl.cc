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

#ifdef COMPILE_BLOBS
#include "natives_blob.cc"
#include "snapshot_blob.cc"
#endif

#define GLOBAL_PRINT "print"
#define GLOBAL_QUIT "quit"
#define GLOBAL_PAUSE "pause"
#define GLOBAL_TITLE "windowTitle"
#define GLOBAL_SW "screenWidth"
#define GLOBAL_SH "screenHeight"
#define GLOBAL_READFILE "readFile"
#define GLOBAL_WRITEFILE "writeFile"
#define GLOBAL_APPENDFILE "appendFile"
#define GLOBAL_INIT "init"
#define GLOBAL_UPDATE "update"
#define GLOBAL_EXIT "exit"
#define GLOBAL_FRAMEBUFFER "screen"

using namespace v8;

int screenWidth, screenHeight;
bool pause = 0;
bool quit = 0;
double fps = 50.0;

class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
    public:
    virtual void* Allocate(size_t length) {
        void* data = AllocateUninitialized(length);
        return data == NULL ? data : memset(data, 0, length);
    }
    virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
    virtual void Free(void* data, size_t) { free(data); }
};

void Log(const char* event) {
    printf("LOG: %s\n", event);
}

static void LogCallback(const FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() < 1) return;
    HandleScope scope(args.GetIsolate());
    Local<Value> arg = args[0];
    String::Utf8Value value(arg);
    
    Log(*value);
}


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
    FILE* file = fopen(name, "wb");
    if (file == NULL) return false;
    fwrite(data, 1, size, file);
    return true;
}

bool FileAppend(const char* name, char* data, size_t size) {
    FILE* file = fopen(name, "ab");
    if (file == NULL) return false;
    fwrite(data, 1, size, file);
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
    
    return context->Global()->Get(context, jsFnName).ToLocal(&fnVal) || fnVal->IsFunction();
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
    EscapableHandleScope handle_scope(isolate);
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
    EscapableHandleScope handle_scope(isolate);
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
    EscapableHandleScope handle_scope(isolate);
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
    int N = screenWidth*screenHeight;
    for (i=0; i<N; i++) {
        pixels[i] = 0x00000000;
    }
}

//void updateScreen(uint32_t* pixels, float t) {}

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

/*
int main(int argc, char *argv[]) {

    screenW = 800;
    screenH = 600;

    SDL_Window *win = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *screen = NULL;

    SDL_Init(SDL_INIT_VIDEO);

    win = SDL_CreateWindow("SDL V8", posX, posY, screenW, screenH, 0);

    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);

    screen = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                screenW, screenH);

    uint32_t* pixels = (uint32_t*) malloc(sizeof(uint32_t)*screenW*screenH);
    float t = 0;
    uint32_t beforeT, afterT;

    while (!quit) {

        beforeT = SDL_GetTicks();
        
        t++;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                handleQuit();
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                handleMouseDown(&e.button);
            } else if (e.type == SDL_MOUSEBUTTONUP) {
                handleMouseUp(&e.button);
            } else  if (e.type == SDL_MOUSEMOTION) {
                handleMouseMove(&e.motion);
            } else if (e.type == SDL_KEYDOWN) {
                handleKeyDown(&e.key);
            } else if (e.type == SDL_KEYUP) {
                handleKeyUp(&e.key);
            }
        }
        
        if (!pause) {

            clearScreen(pixels);

            updateScreen(pixels, t);

            SDL_UpdateTexture(screen, NULL, pixels, screenW * sizeof (uint32_t));

            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, screen, NULL, NULL);
            SDL_RenderPresent(renderer);

            afterT = SDL_GetTicks();

            if (afterT - beforeT < 1000.0/fps) {
                SDL_Delay(1000.0/fps - (afterT - beforeT));
                //printf("delay: %f\n", 1000.0/fps - (afterT - beforeT));
            }
        } else {
            SDL_Delay(20);
        }
    }

    SDL_DestroyTexture(screen);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);

    SDL_Quit();

    return 0;
}
*/

bool ExecGlobFn(Isolate* isolate, const char* fn_name, bool checkExists=false) {
    Local<Context> context(isolate->GetCurrentContext());
    TryCatch try_catch(isolate);
    
    if (checkExists) {
        if (!CheckFnExists(isolate, "process")) {
            printf("Error: no \"%s\" function found\n", fn_name);
            return false;
        }
    }
    // Get function
    Local<Function> fn = GetFn(isolate, fn_name);
    // Invoke the process function, giving the global object as 'this'
    const unsigned argc = 1;
    Local<Value> argv[argc] = { Null(isolate) };

    Local<Value> result;
    
    if (!fn->Call(context, context->Global(), argc, argv).ToLocal(&result)) {
        String::Utf8Value error(try_catch.Exception());
        printf("Exception in function \"%s\": %s\n", fn_name, *error);
        return false;
    } else {
        return true;
    }
}

#define ENGINE_API(js_name, cpp_name) \
    (context->Global()->Set(String::NewFromUtf8(isolate, js_name, NewStringType::kNormal).ToLocalChecked(), \
                            FunctionTemplate::New(isolate, cpp_name)->GetFunction()));

int main(int argc, char* argv[]) {
    
    if (argc < 2 || !FileExists(argv[1])) {
        printf("Valid usage: ./engine file.js\n");
        printf("No source file given, exiting\n");
        return -1;
    }
    
    const char* source = FileRead(argv[1]);
    
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
        
        ExecuteScript(isolate, source);
        
        bool noFnFound = false;
        
        if (!CheckFnExists(isolate, GLOBAL_INIT)) {
            noFnFound = true; printf("No '" GLOBAL_INIT "' function found. '" GLOBAL_INIT "' is required by the engine, please, create it\n");
        }
        if (!CheckFnExists(isolate, GLOBAL_UPDATE)) {
            noFnFound = true; printf("No '" GLOBAL_UPDATE "' function found. '" GLOBAL_UPDATE "' is required by the engine, please, create it\n");
        }
        if (!CheckFnExists(isolate, GLOBAL_EXIT)) {
            noFnFound = true; printf("No '" GLOBAL_EXIT "' function found. '" GLOBAL_EXIT "' is required by the engine, please, create it\n");
        }
        
        if (noFnFound) { printf("Exiting\n"); return -1; }
        
        ExecGlobFn(isolate, GLOBAL_INIT);
        
        bool noGraphics = false;
        
        //Try to init SDL2 window
        screenWidth = (int) GetNumber(isolate, GLOBAL_SW, 640);
        screenHeight = (int) GetNumber(isolate, GLOBAL_SH, 480);
        const char* windowTitle = GetString(isolate, GLOBAL_TITLE, "SDL2 V8 Application");
        
        SDL_Window *win = NULL;
        SDL_Renderer *renderer = NULL;
        SDL_Texture *screen = NULL;
        
        SDL_Init(SDL_INIT_VIDEO);
        
        win = SDL_CreateWindow(windowTitle, 0, 0, screenWidth, screenHeight, 0);
        renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
        screen = SDL_CreateTexture(renderer,
                                   SDL_PIXELFORMAT_ARGB8888,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   screenWidth, screenHeight);
        
        size_t fbSize = sizeof(uint32_t)*screenWidth*screenHeight;
        uint32_t* pixels = (uint32_t*) malloc(fbSize);
        float t = 0;
        uint32_t beforeT, afterT;
        
        makeGlobalByteArray(isolate, GLOBAL_FRAMEBUFFER, fbSize, pixels);
        
        if (noGraphics) {
            printf("ERROR: Couldn't init graphics subsystem, perhaps you forgot to set some global variables\nExiting\n");
            return -1;
        }
        
        quit = 0;
        
        // Mainloop
        while (!quit) {
                
            beforeT = SDL_GetTicks();
            
            t++;

            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    handleQuit();
                } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                    handleMouseDown(&e.button);
                } else if (e.type == SDL_MOUSEBUTTONUP) {
                    handleMouseUp(&e.button);
                } else  if (e.type == SDL_MOUSEMOTION) {
                    handleMouseMove(&e.motion);
                } else if (e.type == SDL_KEYDOWN) {
                    handleKeyDown(&e.key);
                } else if (e.type == SDL_KEYUP) {
                    handleKeyUp(&e.key);
                }
            }
            
            if (!pause) {
                
                clearScreen(pixels);
                
                ExecGlobFn(isolate, GLOBAL_UPDATE);
                
                quit = quit || GetBoolean(isolate, GLOBAL_QUIT);
                
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

        return 0;
        
        /*
        size_t testSize = 1024;
        char* screen = (char*) malloc(1024);
        memset(testData, 0, 1);
        
        makeGlobalByteArray(isolate, "testData", testSize, testData);
        
        double sw = GetNumber(isolate, "screenWidth", 640);
        
        printf("SYSTEM: screenWidth=%f\n", sw);
        
        const char* wt = GetString(isolate, "windowTitle", "V8 SDL2");
    
        printf("SYSTEM: windowTitle=%s\n", wt);
        
        printf("SYSTEM: true=%d\n", GetBoolean(isolate, "testBool1"));
        printf("SYSTEM: false=%d\n", GetBoolean(isolate, "testBool2"));
        printf("SYSTEM: undefined=%d\n", GetBoolean(isolate, "testBool3"));
        printf("SYSTEM: undeclared=%d\n", GetBoolean(isolate, "testBool4"));
        
        // Set up an exception handler before calling the Process function
        TryCatch try_catch(isolate);

        if (CheckFnExists(isolate, "process")) {
            printf("JavaScript Function found: \"process\"\n");
        } else {
            printf("Error: no \"process\" function found\n");
            return 1;
        }

        // Get function
        Local<Function> process_fun = GetFn(isolate, "process");

        // Invoke the process function, giving the global object as 'this'
        // and one argument, the request.

        const unsigned argc = 2;
        Local<Value> argv[argc] = { Null(isolate), String::NewFromUtf8(isolate, "success") };

        //Local<Function> process = v8::Local<v8::Function>::New(isolate, process_);

        Local<Value> result;

        if (!process_fun->Call(context, context->Global(), argc, argv).ToLocal(&result)) {
            String::Utf8Value error(try_catch.Exception());
            printf("Error: %s\n", *error);
            return false;
        } else {
            return true;
        }
        */
    }

    // Dispose the isolate and tear down V8.
    isolate->Dispose();
    V8::Dispose();
    V8::ShutdownPlatform();
    delete platform;
    return 0;
}
