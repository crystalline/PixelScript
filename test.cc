// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//g++ -std=c++11 -I"v8/include" test.cc -o test -Wl,--start-group \
v8/out/x64.release/obj.target/{tools/gyp/libv8_{base,libbase,external_snapshot,libplatform},third_party/icu/libicu{uc,i18n,data}}.a -Wl,--end-group \
-lrt -ldl -pthread


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "v8/include/libplatform/libplatform.h"
#include "v8/include/v8.h"

using namespace v8;

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

static void LogCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() < 1) return;
    HandleScope scope(args.GetIsolate());
    Local<Value> arg = args[0];
    String::Utf8Value value(arg);
    Log(*value);
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
    
    String::Utf8Value utf8(result);
    printf("%s\n", *utf8);
  
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

    // The script compiled and ran correctly.  Now we fetch out the
    // Process function from the global object.
    Local<String> process_name =
        String::NewFromUtf8(isolate, fnName, NewStringType::kNormal)
        .ToLocalChecked();
    
    Local<Value> process_val;
    
    // If there is no Process function, or if it is not a function,
    // bail out
    return context->Global()->Get(context, process_name).ToLocal(&process_val) || !process_val->IsFunction();
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

int main(int argc, char* argv[]) {
  // Initialize V8.
  V8::InitializeICU();
  V8::InitializeExternalStartupData(argv[0]);
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
    
    Local<ObjectTemplate> global = ObjectTemplate::New(isolate);
    global->Set(String::NewFromUtf8(isolate, "log", NewStringType::kNormal).ToLocalChecked(),
                FunctionTemplate::New(isolate, LogCallback));
    
    // Create a new context.
    Local<Context> context = Context::New(isolate, NULL, global);
    
    // Enter the context for compiling and running the hello world script.
    Context::Scope context_scope(context);
    
    ExecuteScript(isolate, "function process() { log('Hello, Log callback called from JavaScript!'); return 1 }; 'Hello' + ', World!'");

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
    
  }

  // Dispose the isolate and tear down V8.
  isolate->Dispose();
  V8::Dispose();
  V8::ShutdownPlatform();
  delete platform;
  return 0;
}
