// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "include/dart_api.h"

#include "vm/bigint_operations.h"
#include "vm/class_finalizer.h"
#include "vm/compiler.h"
#include "vm/dart.h"
#include "vm/dart_api_impl.h"
#include "vm/dart_api_state.h"
#include "vm/dart_entry.h"
#include "vm/debuginfo.h"
#include "vm/exceptions.h"
#include "vm/growable_array.h"
#include "vm/longjump.h"
#include "vm/native_entry.h"
#include "vm/object.h"
#include "vm/object_store.h"
#include "vm/port.h"
#include "vm/resolver.h"
#include "vm/snapshot.h"
#include "vm/stack_frame.h"
#include "vm/timer.h"
#include "vm/verifier.h"

namespace dart {

const char* CanonicalFunction(const char* func) {
  if (strncmp(func, "dart::", 6) == 0) {
    return func + 6;
  } else {
    return func;
  }
}

#define RETURN_TYPE_ERROR(dart_handle, Type)                                  \
  do {                                                                        \
    const Object& tmp = Object::Handle(Api::UnwrapHandle((dart_handle)));     \
    if (tmp.IsNull()) {                                                       \
      return Api::Error("%s expects argument '%s' to be non-null.",           \
                        CURRENT_FUNC, #dart_handle);                          \
    } else if (tmp.IsApiError()) {                                            \
      return dart_handle;                                                     \
    } else {                                                                  \
      return Api::Error("%s expects argument '%s' to be of type %s.",         \
                        CURRENT_FUNC, #dart_handle, #Type);                   \
    }                                                                         \
  } while (0)


// Return error if isolate is in an inconsistent state.
// Return NULL when no error condition exists.
const char* CheckIsolateState(Isolate* isolate, bool generating_snapshot) {
  bool success = (generating_snapshot) ?
      ClassFinalizer::FinalizePendingClassesForSnapshotCreation() :
      ClassFinalizer::FinalizePendingClasses();
  if (success && !generating_snapshot) {
    success = isolate->object_store()->PreallocateObjects();
  }
  if (success) {
    return NULL;
  } else {
    // Make a copy of the error message as the original message string
    // may get deallocated when we return back from the Dart API call.
    const String& err =
    String::Handle(isolate->object_store()->sticky_error());
    const char* errmsg = err.ToCString();
    intptr_t errlen = strlen(errmsg) + 1;
    char* msg = reinterpret_cast<char*>(Api::Allocate(errlen));
    OS::SNPrint(msg, errlen, "%s", errmsg);
    return msg;
  }
}


void SetupErrorResult(Dart_Handle* handle) {
  // Make a copy of the error message as the original message string
  // may get deallocated when we return back from the Dart API call.
  const String& error = String::Handle(
      Isolate::Current()->object_store()->sticky_error());
  const Object& obj = Object::Handle(ApiError::New(error));
  *handle = Api::NewLocalHandle(obj);
}


// NOTE: Need to pass 'result' as a parameter here in order to avoid
// warning: variable 'result' might be clobbered by 'longjmp' or 'vfork'
// which shows up because of the use of setjmp.
static void InvokeStatic(Isolate* isolate,
                         const Function& function,
                         GrowableArray<const Object*>& args,
                         Dart_Handle* result) {
  ASSERT(isolate != NULL);
  LongJump* base = isolate->long_jump_base();
  LongJump jump;
  isolate->set_long_jump_base(&jump);
  if (setjmp(*jump.Set()) == 0) {
    const Array& kNoArgumentNames = Array::Handle();
    const Instance& retval = Instance::Handle(
        DartEntry::InvokeStatic(function, args, kNoArgumentNames));
    if (retval.IsUnhandledException()) {
      *result = Api::ErrorFromException(retval);
    } else {
      *result = Api::NewLocalHandle(retval);
    }
  } else {
    SetupErrorResult(result);
  }
  isolate->set_long_jump_base(base);
}


// NOTE: Need to pass 'result' as a parameter here in order to avoid
// warning: variable 'result' might be clobbered by 'longjmp' or 'vfork'
// which shows up because of the use of setjmp.
static void InvokeDynamic(Isolate* isolate,
                          const Instance& receiver,
                          const Function& function,
                          GrowableArray<const Object*>& args,
                          Dart_Handle* result) {
  ASSERT(isolate != NULL);
  LongJump* base = isolate->long_jump_base();
  LongJump jump;
  isolate->set_long_jump_base(&jump);
  if (setjmp(*jump.Set()) == 0) {
    const Array& kNoArgumentNames = Array::Handle();
    const Instance& retval = Instance::Handle(
        DartEntry::InvokeDynamic(receiver, function, args, kNoArgumentNames));
    if (retval.IsUnhandledException()) {
      *result = Api::ErrorFromException(retval);
    } else {
      *result = Api::NewLocalHandle(retval);
    }
  } else {
    SetupErrorResult(result);
  }
  isolate->set_long_jump_base(base);
}


Dart_Handle Api::NewLocalHandle(const Object& object) {
  Isolate* isolate = Isolate::Current();
  ASSERT(isolate != NULL);
  ApiState* state = isolate->api_state();
  ASSERT(state != NULL);
  ApiLocalScope* scope = state->top_scope();
  ASSERT(scope != NULL);
  LocalHandles* local_handles = scope->local_handles();
  ASSERT(local_handles != NULL);
  LocalHandle* ref = local_handles->AllocateHandle();
  ref->set_raw(object);
  return reinterpret_cast<Dart_Handle>(ref);
}

RawObject* Api::UnwrapHandle(Dart_Handle object) {
#ifdef DEBUG
  Isolate* isolate = Isolate::Current();
  ASSERT(isolate != NULL);
  ApiState* state = isolate->api_state();
  ASSERT(state != NULL);
  ASSERT(state->IsValidPersistentHandle(object) ||
         state->IsValidLocalHandle(object));
  ASSERT(PersistentHandle::raw_offset() == 0 &&
         LocalHandle::raw_offset() == 0);
#endif
  return *(reinterpret_cast<RawObject**>(object));
}

#define DEFINE_UNWRAP(Type)                                                   \
  const Type& Api::Unwrap##Type##Handle(Dart_Handle dart_handle) {            \
    const Object& tmp = Object::Handle(Api::UnwrapHandle(dart_handle));       \
    Type& typed_handle = Type::Handle();                                      \
    if (tmp.Is##Type()) {                                                     \
      typed_handle ^= tmp.raw();                                              \
    }                                                                         \
    return typed_handle;                                                      \
  }
CLASS_LIST_NO_OBJECT(DEFINE_UNWRAP)
#undef DEFINE_UNWRAP


LocalHandle* Api::UnwrapAsLocalHandle(const ApiState& state,
                                      Dart_Handle object) {
  ASSERT(state.IsValidLocalHandle(object));
  return reinterpret_cast<LocalHandle*>(object);
}


PersistentHandle* Api::UnwrapAsPersistentHandle(const ApiState& state,
                                                Dart_Handle object) {
  ASSERT(state.IsValidPersistentHandle(object));
  return reinterpret_cast<PersistentHandle*>(object);
}


Dart_Isolate Api::CastIsolate(Isolate* isolate) {
  return reinterpret_cast<Dart_Isolate>(isolate);
}


Dart_Message Api::CastMessage(uint8_t* message) {
  return reinterpret_cast<Dart_Message>(message);
}


Dart_Handle Api::Success() {
  Isolate* isolate = Isolate::Current();
  ASSERT(isolate != NULL);
  ApiState* state = isolate->api_state();
  ASSERT(state != NULL);
  PersistentHandle* true_handle = state->True();
  return reinterpret_cast<Dart_Handle>(true_handle);
}


Dart_Handle Api::Error(const char* format, ...) {
  DARTSCOPE_NOCHECKS(Isolate::Current());

  va_list args;
  va_start(args, format);
  intptr_t len = OS::VSNPrint(NULL, 0, format, args);
  va_end(args);

  char* buffer = reinterpret_cast<char*>(zone.Allocate(len + 1));
  va_list args2;
  va_start(args2, format);
  OS::VSNPrint(buffer, (len + 1), format, args2);
  va_end(args2);

  const String& message = String::Handle(String::New(buffer));
  const Object& obj = Object::Handle(ApiError::New(message));
  return Api::NewLocalHandle(obj);
}


Dart_Handle Api::ErrorFromException(const Object& obj) {
  DARTSCOPE_NOCHECKS(Isolate::Current());

  ASSERT(obj.IsUnhandledException());
  if (obj.IsUnhandledException()) {
    UnhandledException& uhe = UnhandledException::Handle();
    uhe ^= obj.raw();
    const Object& error = Object::Handle(ApiError::New(uhe));
    return Api::NewLocalHandle(error);
  } else {
    return Api::Error("Internal error: expected obj.IsUnhandledException().");
  }
}


Dart_Handle Api::Null() {
  Isolate* isolate = Isolate::Current();
  ASSERT(isolate != NULL);
  ApiState* state = isolate->api_state();
  ASSERT(state != NULL);
  PersistentHandle* null_handle = state->Null();
  return reinterpret_cast<Dart_Handle>(null_handle);
}


Dart_Handle Api::True() {
  Isolate* isolate = Isolate::Current();
  ASSERT(isolate != NULL);
  ApiState* state = isolate->api_state();
  ASSERT(state != NULL);
  PersistentHandle* true_handle = state->True();
  return reinterpret_cast<Dart_Handle>(true_handle);
}


Dart_Handle Api::False() {
  Isolate* isolate = Isolate::Current();
  ASSERT(isolate != NULL);
  ApiState* state = isolate->api_state();
  ASSERT(state != NULL);
  PersistentHandle* false_handle = state->False();
  return reinterpret_cast<Dart_Handle>(false_handle);
}


uword Api::Allocate(intptr_t size) {
  Isolate* isolate = Isolate::Current();
  ASSERT(isolate != NULL);
  ApiState* state = isolate->api_state();
  ASSERT(state != NULL);
  ApiLocalScope* scope = state->top_scope();
  ASSERT(scope != NULL);
  return scope->zone().Allocate(size);
}


uword Api::Reallocate(uword ptr, intptr_t old_size, intptr_t new_size) {
  Isolate* isolate = Isolate::Current();
  ASSERT(isolate != NULL);
  ApiState* state = isolate->api_state();
  ASSERT(state != NULL);
  ApiLocalScope* scope = state->top_scope();
  ASSERT(scope != NULL);
  return scope->zone().Reallocate(ptr, old_size, new_size);
}


// --- Handles ---


DART_EXPORT bool Dart_IsError(const Dart_Handle& handle) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(handle));
  return obj.IsApiError();
}


static const char* MakeUnhandledExceptionCString(
    const UnhandledException& uhe) {
  const Instance& exception = Instance::Handle(uhe.exception());
  Object& strtmp = Object::Handle(DartLibraryCalls::ToString(exception));
  const char* exc_str =
      "<Received exception while converting exception to string>";
  if (!strtmp.IsUnhandledException()) {
    exc_str = strtmp.ToCString();
  }

  const Instance& stack = Instance::Handle(uhe.stacktrace());
  strtmp = DartLibraryCalls::ToString(stack);
  const char* stack_str =
      "<Received exception while converting stack trace to string>";
  if (!strtmp.IsUnhandledException()) {
    stack_str = strtmp.ToCString();
  }

  const char* format = "Unhandled exception:\n%s\n%s";
  int len = (strlen(exc_str) + strlen(stack_str) + strlen(format)
             - 4    // Two '%s'
             + 1);  // '\0'
  char* buffer = reinterpret_cast<char*>(Api::Allocate(len));
  OS::SNPrint(buffer, len, format, exc_str, stack_str);
  return buffer;
}


DART_EXPORT const char* Dart_GetError(const Dart_Handle& handle) {
  DARTSCOPE(Isolate::Current());

  const Object& obj = Object::Handle(Api::UnwrapHandle(handle));
  if (!obj.IsApiError()) {
    return "";
  }
  ApiError& failure = ApiError::Handle();
  failure ^= obj.raw();
  const Object& data = Object::Handle(failure.data());
  if (data.IsString()) {
    // Simple error message.
    String& message = String::Handle();
    message ^= failure.data();
    const char* msg = message.ToCString();
    intptr_t len = strlen(msg) + 1;
    char* msg_copy = reinterpret_cast<char*>(Api::Allocate(len));
    strncpy(msg_copy, msg, len);
    return msg_copy;

  } else if (data.IsUnhandledException()) {
    UnhandledException& uhe = UnhandledException::Handle();
    uhe ^= data.raw();
    return MakeUnhandledExceptionCString(uhe);

  } else {
    return "<Internal error in Dart_GetError: malformed error handle>";
  }
}


DART_EXPORT bool Dart_ErrorHasException(Dart_Handle handle) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(handle));
  if (obj.IsApiError()) {
    const ApiError& error = ApiError::CheckedHandle(obj.raw());
    const Object& data = Object::Handle(error.data());
    return data.IsUnhandledException();
  }
  return false;
}


DART_EXPORT Dart_Handle Dart_ErrorGetException(Dart_Handle handle) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(handle));
  if (obj.IsApiError()) {
    const ApiError& error = ApiError::CheckedHandle(obj.raw());
    const Object& data = Object::Handle(error.data());
    if (data.IsUnhandledException()) {
      const UnhandledException& unhandled = UnhandledException::Handle(
          reinterpret_cast<RawUnhandledException*>(data.raw()));
      const Object& exception = Object::Handle(unhandled.exception());
      return Api::NewLocalHandle(exception);
    } else {
      return Api::Error("This error is not an unhandled exception error.");
    }
  } else {
    return Api::Error("Can only get exceptions from error handles.");
  }
}


DART_EXPORT Dart_Handle Dart_ErrorGetStacktrace(Dart_Handle handle) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(handle));
  if (obj.IsApiError()) {
    ApiError& failure = ApiError::Handle();
    failure ^= obj.raw();
    const Object& data = Object::Handle(failure.data());
    if (data.IsUnhandledException()) {
      const UnhandledException& unhandled = UnhandledException::Handle(
          reinterpret_cast<RawUnhandledException*>(data.raw()));
      const Object& stacktrace = Object::Handle(unhandled.stacktrace());
      return Api::NewLocalHandle(stacktrace);
    } else {
      return Api::Error("This error is not an unhandled exception error.");
    }
  } else {
    return Api::Error("Can only get stacktraces from error handles.");
  }
}


// TODO(turnidge): This clones Api::Error.  I need to use va_copy to
// fix this but not sure if it available on all of our builds.
DART_EXPORT Dart_Handle Dart_Error(const char* format, ...) {
  DARTSCOPE(Isolate::Current());

  va_list args;
  va_start(args, format);
  intptr_t len = OS::VSNPrint(NULL, 0, format, args);
  va_end(args);

  char* buffer = reinterpret_cast<char*>(zone.Allocate(len + 1));
  va_list args2;
  va_start(args2, format);
  OS::VSNPrint(buffer, (len + 1), format, args2);
  va_end(args2);

  const String& message = String::Handle(String::New(buffer));
  const Object& obj = Object::Handle(ApiError::New(message));
  return Api::NewLocalHandle(obj);
}


DART_EXPORT void _Dart_ReportErrorHandle(const char* file,
                                           int line,
                                           const char* handle,
                                           const char* message) {
  fprintf(stderr, "%s:%d: error handle: '%s':\n    '%s'\n",
          file, line, handle, message);
  OS::Abort();
}


DART_EXPORT Dart_Handle Dart_ToString(Dart_Handle object) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(object));
  Object& result = Object::Handle();
  if (obj.IsString()) {
    result = obj.raw();
  } else if (obj.IsInstance()) {
    Instance& receiver = Instance::Handle();
    receiver ^= obj.raw();
    result = DartLibraryCalls::ToString(receiver);
    if (result.IsUnhandledException()) {
      return Api::ErrorFromException(result);
    }
  } else {
    // This is a VM internal object. Call the C++ method of printing.
    result = String::New(obj.ToCString());
  }
  return Api::NewLocalHandle(result);
}


DART_EXPORT Dart_Handle Dart_IsSame(Dart_Handle obj1, Dart_Handle obj2,
                                    bool* value) {
  DARTSCOPE(Isolate::Current());
  const Object& expected = Object::Handle(Api::UnwrapHandle(obj1));
  const Object& actual = Object::Handle(Api::UnwrapHandle(obj2));
  *value = (expected.raw() == actual.raw());
  return Api::Success();
}


DART_EXPORT Dart_Handle Dart_NewPersistentHandle(Dart_Handle object) {
  Isolate* isolate = Isolate::Current();
  CHECK_ISOLATE(isolate);
  DARTSCOPE_NOCHECKS(isolate);
  ApiState* state = isolate->api_state();
  ASSERT(state != NULL);
  const Object& old_ref = Object::Handle(Api::UnwrapHandle(object));
  PersistentHandle* new_ref = state->persistent_handles().AllocateHandle();
  new_ref->set_raw(old_ref);
  return reinterpret_cast<Dart_Handle>(new_ref);
}


DART_EXPORT void Dart_DeletePersistentHandle(Dart_Handle object) {
  Isolate* isolate = Isolate::Current();
  CHECK_ISOLATE(isolate);
  ApiState* state = isolate->api_state();
  ASSERT(state != NULL);
  PersistentHandle* ref = Api::UnwrapAsPersistentHandle(*state, object);
  ASSERT(!ref->IsProtected());
  if (!ref->IsProtected()) {
    state->persistent_handles().FreeHandle(ref);
  }
}


DART_EXPORT Dart_Handle Dart_MakeWeakPersistentHandle(Dart_Handle object) {
  UNIMPLEMENTED();
  return NULL;
}


DART_EXPORT Dart_Handle Dart_MakePersistentHandle(Dart_Handle object) {
  UNIMPLEMENTED();
  return NULL;
}


// --- Initialization and Globals ---


// TODO(iposva): This is a placeholder for the eventual external Dart API.
DART_EXPORT bool Dart_Initialize(Dart_IsolateCreateCallback callback) {
  return Dart::InitOnce(callback);
}

DART_EXPORT bool Dart_SetVMFlags(int argc, const char** argv) {
  return Flags::ProcessCommandLineFlags(argc, argv);
}

DART_EXPORT bool Dart_IsVMFlagSet(const char* flag_name) {
  if (Flags::Lookup(flag_name) != NULL) {
    return true;
  }
  return false;
}


// --- Isolates ---


DART_EXPORT Dart_Isolate Dart_CreateIsolate(const uint8_t* snapshot,
                                            void* callback_data,
                                            char** error) {
  Isolate* isolate = Dart::CreateIsolate();
  assert(isolate != NULL);
  LongJump* base = isolate->long_jump_base();
  LongJump jump;
  isolate->set_long_jump_base(&jump);
  if (setjmp(*jump.Set()) == 0) {
    Dart::InitializeIsolate(snapshot, callback_data);
    START_TIMER(time_total_runtime);
    isolate->set_long_jump_base(base);
    return reinterpret_cast<Dart_Isolate>(isolate);
  } else {
    {
      DARTSCOPE_NOCHECKS(isolate);
      const String& errmsg =
          String::Handle(isolate->object_store()->sticky_error());
      *error = strdup(errmsg.ToCString());
    }
    Dart::ShutdownIsolate();
  }
  return reinterpret_cast<Dart_Isolate>(NULL);
}


DART_EXPORT void Dart_ShutdownIsolate() {
  CHECK_ISOLATE(Isolate::Current());
  STOP_TIMER(time_total_runtime);
  Dart::ShutdownIsolate();
}


DART_EXPORT Dart_Isolate Dart_CurrentIsolate() {
  return Api::CastIsolate(Isolate::Current());
}


DART_EXPORT void Dart_EnterIsolate(Dart_Isolate dart_isolate) {
  CHECK_NO_ISOLATE(Isolate::Current());
  Isolate* isolate = reinterpret_cast<Isolate*>(dart_isolate);
  Isolate::SetCurrent(isolate);
}


DART_EXPORT void Dart_ExitIsolate() {
  CHECK_ISOLATE(Isolate::Current());
  Isolate::SetCurrent(NULL);
}


static uint8_t* ApiAllocator(uint8_t* ptr,
                             intptr_t old_size,
                             intptr_t new_size) {
  uword new_ptr = Api::Reallocate(reinterpret_cast<uword>(ptr),
                                  old_size,
                                  new_size);
  return reinterpret_cast<uint8_t*>(new_ptr);
}


DART_EXPORT Dart_Handle Dart_CreateSnapshot(uint8_t** buffer,
                                            intptr_t* size) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  if (buffer == NULL) {
    return Api::Error("%s expects argument 'buffer' to be non-null.",
                      CURRENT_FUNC);
  }
  if (size == NULL) {
    return Api::Error("%s expects argument 'size' to be non-null.",
                      CURRENT_FUNC);
  }
  const char* msg = CheckIsolateState(isolate,
                                      ClassFinalizer::kGeneratingSnapshot);
  if (msg != NULL) {
    return Api::Error(msg);
  }
  // Since this is only a snapshot the root library should not be set.
  isolate->object_store()->set_root_library(Library::Handle());
  SnapshotWriter writer(Snapshot::kFull, buffer, ApiAllocator);
  writer.WriteFullSnapshot();
  *size = writer.Size();
  return Api::Success();
}


DART_EXPORT Dart_Handle Dart_CreateScriptSnapshot(Dart_Handle library,
                                                  uint8_t** buffer,
                                                  intptr_t* size) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  if (buffer == NULL) {
    return Api::Error("%s expects argument 'buffer' to be non-null.",
                      CURRENT_FUNC);
  }
  if (size == NULL) {
    return Api::Error("%s expects argument 'size' to be non-null.",
                      CURRENT_FUNC);
  }
  const Library& root_lib = Api::UnwrapLibraryHandle(library);
  if (root_lib.IsNull()) {
    RETURN_TYPE_ERROR(library, Library);
  }
  const char* msg = CheckIsolateState(isolate);
  if (msg != NULL) {
    return Api::Error(msg);
  }
  ScriptSnapshotWriter writer(root_lib, buffer, ApiAllocator);
  writer.WriteScriptSnapshot();
  *size = writer.Size();
  return Api::Success();
}


// --- Messages and Ports ---


DART_EXPORT void Dart_SetMessageCallbacks(
    Dart_PostMessageCallback post_message_callback,
    Dart_ClosePortCallback close_port_callback) {
  Isolate* isolate = Isolate::Current();
  CHECK_ISOLATE(isolate);
  ASSERT(post_message_callback != NULL);
  ASSERT(close_port_callback != NULL);
  isolate->set_post_message_callback(post_message_callback);
  isolate->set_close_port_callback(close_port_callback);
}


static RawInstance* DeserializeMessage(void* data) {
  // Create a snapshot object using the buffer.
  const Snapshot* snapshot = Snapshot::SetupFromBuffer(data);
  ASSERT(snapshot->IsMessageSnapshot());

  // Read object back from the snapshot.
  Isolate* isolate = Isolate::Current();
  SnapshotReader reader(snapshot, isolate->heap(), isolate->object_store());
  Instance& instance = Instance::Handle();
  instance ^= reader.ReadObject();
  return instance.raw();
}


DART_EXPORT Dart_Handle Dart_HandleMessage(Dart_Port dest_port_id,
                                           Dart_Port reply_port_id,
                                           Dart_Message dart_message) {
  DARTSCOPE(Isolate::Current());

  const Instance& msg = Instance::Handle(DeserializeMessage(dart_message));
  const String& class_name =
      String::Handle(String::NewSymbol("ReceivePortImpl"));
  const String& function_name =
      String::Handle(String::NewSymbol("_handleMessage"));
  const int kNumArguments = 3;
  const Array& kNoArgumentNames = Array::Handle();
  const Function& function = Function::Handle(
      Resolver::ResolveStatic(Library::Handle(Library::CoreLibrary()),
                              class_name,
                              function_name,
                              kNumArguments,
                              kNoArgumentNames,
                              Resolver::kIsQualified));
  GrowableArray<const Object*> arguments(kNumArguments);
  arguments.Add(&Integer::Handle(Integer::New(dest_port_id)));
  arguments.Add(&Integer::Handle(Integer::New(reply_port_id)));
  arguments.Add(&msg);
  // TODO(turnidge): This call should be wrapped in a longjmp
  const Object& result = Object::Handle(
      DartEntry::InvokeStatic(function, arguments, kNoArgumentNames));
  if (result.IsUnhandledException()) {
    return Api::ErrorFromException(result);
  }
  ASSERT(result.IsNull());
  return Api::Success();
}


DART_EXPORT Dart_Handle Dart_RunLoop() {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);

  LongJump* base = isolate->long_jump_base();
  LongJump jump;
  Dart_Handle result;
  isolate->set_long_jump_base(&jump);
  if (setjmp(*jump.Set()) == 0) {
    isolate->StandardRunLoop();
    result = Api::Success();
  } else {
    SetupErrorResult(&result);
  }
  isolate->set_long_jump_base(base);
  return result;
}


DART_EXPORT bool Dart_HasLivePorts() {
  Isolate* isolate = Isolate::Current();
  ASSERT(isolate);
  return isolate->live_ports() > 0;
}


static uint8_t* allocator(uint8_t* ptr, intptr_t old_size, intptr_t new_size) {
  void* new_ptr = realloc(reinterpret_cast<void*>(ptr), new_size);
  return reinterpret_cast<uint8_t*>(new_ptr);
}


DART_EXPORT bool Dart_PostIntArray(Dart_Port port_id,
                                   intptr_t len,
                                   intptr_t* data) {
  uint8_t* buffer = NULL;
  MessageWriter writer(&buffer, &allocator);

  writer.WriteMessage(len, data);

  // Post the message at the given port.
  return PortMap::PostMessage(port_id, kNoReplyPort, Api::CastMessage(buffer));
}


DART_EXPORT bool Dart_Post(Dart_Port port_id, Dart_Handle handle) {
  Isolate* isolate = Isolate::Current();
  CHECK_ISOLATE(isolate);
  DARTSCOPE_NOCHECKS(isolate);
  const Object& object = Object::Handle(Api::UnwrapHandle(handle));
  uint8_t* data = NULL;
  SnapshotWriter writer(Snapshot::kMessage, &data, &allocator);
  writer.WriteObject(object.raw());
  writer.FinalizeBuffer();
  return PortMap::PostMessage(port_id, kNoReplyPort, Api::CastMessage(data));
}


DART_EXPORT Dart_Handle Dart_NewSendPort(Dart_Port port_id) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  const String& class_name = String::Handle(String::NewSymbol("SendPortImpl"));
  const String& function_name = String::Handle(String::NewSymbol("_create"));
  const int kNumArguments = 1;
  const Array& kNoArgumentNames = Array::Handle();
  // TODO(turnidge): Consider adding a helper function to make
  // function resolution by class name and function name more concise.
  const Function& function = Function::Handle(
      Resolver::ResolveStatic(Library::Handle(Library::CoreLibrary()),
                              class_name,
                              function_name,
                              kNumArguments,
                              kNoArgumentNames,
                              Resolver::kIsQualified));
  GrowableArray<const Object*> arguments(kNumArguments);
  arguments.Add(&Integer::Handle(Integer::New(port_id)));
  Dart_Handle result;
  InvokeStatic(isolate, function, arguments, &result);
  return result;
}


DART_EXPORT Dart_Handle Dart_GetReceivePort(Dart_Port port_id) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  const String& class_name =
      String::Handle(String::NewSymbol("ReceivePortImpl"));
  const String& function_name =
      String::Handle(String::NewSymbol("_get_or_create"));
  const int kNumArguments = 1;
  const Array& kNoArgumentNames = Array::Handle();
  const Function& function = Function::Handle(
      Resolver::ResolveStatic(Library::Handle(Library::CoreLibrary()),
                              class_name,
                              function_name,
                              kNumArguments,
                              kNoArgumentNames,
                              Resolver::kIsQualified));
  GrowableArray<const Object*> arguments(kNumArguments);
  arguments.Add(&Integer::Handle(Integer::New(port_id)));
  Dart_Handle result;
  InvokeStatic(isolate, function, arguments, &result);
  return result;
}


DART_EXPORT Dart_Port Dart_GetMainPortId() {
  Isolate* isolate = Isolate::Current();
  CHECK_ISOLATE(isolate);
  return isolate->main_port();
}

// --- Scopes ----


DART_EXPORT void Dart_EnterScope() {
  Isolate* isolate = Isolate::Current();
  CHECK_ISOLATE(isolate);
  ApiState* state = isolate->api_state();
  ASSERT(state != NULL);
  ApiLocalScope* new_scope = new ApiLocalScope(state->top_scope(),
                                               reinterpret_cast<uword>(&state));
  ASSERT(new_scope != NULL);
  state->set_top_scope(new_scope);  // New scope is now the top scope.
}


DART_EXPORT void Dart_ExitScope() {
  Isolate* isolate = Isolate::Current();
  CHECK_ISOLATE_SCOPE(isolate);
  ApiState* state = isolate->api_state();
  ApiLocalScope* scope = state->top_scope();

  state->set_top_scope(scope->previous());  // Reset top scope to previous.
  delete scope;  // Free up the old scope which we have just exited.
}


// --- Objects ----


DART_EXPORT Dart_Handle Dart_Null() {
  CHECK_ISOLATE_SCOPE(Isolate::Current());
  return Api::Null();
}


DART_EXPORT bool Dart_IsNull(Dart_Handle object) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(object));
  return obj.IsNull();
}


DART_EXPORT Dart_Handle Dart_ObjectEquals(Dart_Handle obj1, Dart_Handle obj2,
                                          bool* value) {
  DARTSCOPE(Isolate::Current());
  const Instance& expected = Instance::CheckedHandle(Api::UnwrapHandle(obj1));
  const Instance& actual = Instance::CheckedHandle(Api::UnwrapHandle(obj2));
  const Instance& result =
      Instance::Handle(DartLibraryCalls::Equals(expected, actual));
  if (result.IsBool()) {
    Bool& b = Bool::Handle();
    b ^= result.raw();
    *value = b.value();
    return Api::Success();
  } else if (result.IsUnhandledException()) {
    return Api::ErrorFromException(result);
  } else {
    return Api::Error("Expected boolean result from ==");
  }
}


// TODO(iposva): This call actually implements IsInstanceOfClass.
// Do we also need a real Dart_IsInstanceOf, which should take an instance
// rather than an object and a type rather than a class?
DART_EXPORT Dart_Handle Dart_ObjectIsType(Dart_Handle object,
                                          Dart_Handle clazz,
                                          bool* value) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  const Class& cls = Class::CheckedHandle(Api::UnwrapHandle(clazz));
  if (cls.IsNull()) {
    return Api::Error("instanceof check against null class");
  }
  const Object& obj = Object::Handle(Api::UnwrapHandle(object));
  Instance& instance = Instance::Handle();
  instance ^= obj.raw();
  // Finalize all classes.
  const char* msg = CheckIsolateState(isolate);
  if (msg != NULL) {
    return Api::Error(msg);
  }
  const Type& type = Type::Handle(Type::NewNonParameterizedType(cls));
  *value = instance.IsInstanceOf(type, TypeArguments::Handle());
  return Api::Success();
}


// --- Numbers ----


// TODO(iposva): The argument should be an instance.
DART_EXPORT bool Dart_IsNumber(Dart_Handle object) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(object));
  return obj.IsNumber();
}


// --- Integers ----


DART_EXPORT bool Dart_IsInteger(Dart_Handle object) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(object));
  return obj.IsInteger();
}


DART_EXPORT Dart_Handle Dart_IntegerFitsIntoInt64(Dart_Handle integer,
                                                  bool* fits) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(integer));
  if (obj.IsSmi() || obj.IsMint()) {
    *fits = true;
    return Api::Success();
  } else if (obj.IsBigint()) {
#if defined(DEBUG)
    Bigint& bigint = Bigint::Handle();
    bigint ^= obj.raw();
    ASSERT(!BigintOperations::FitsIntoInt64(bigint));
#endif
    *fits = false;
    return Api::Success();
  }
  return Api::Error("Object is not a Integer");
}


DART_EXPORT Dart_Handle Dart_NewInteger(int64_t value) {
  DARTSCOPE(Isolate::Current());
  const Integer& obj = Integer::Handle(Integer::New(value));
  return Api::NewLocalHandle(obj);
}


DART_EXPORT Dart_Handle Dart_NewIntegerFromHexCString(const char* str) {
  DARTSCOPE(Isolate::Current());
  const String& str_obj = String::Handle(String::New(str));
  const Integer& obj = Integer::Handle(Integer::New(str_obj));
  return Api::NewLocalHandle(obj);
}


DART_EXPORT Dart_Handle Dart_IntegerValue(Dart_Handle integer, int64_t* value) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(integer));
  if (obj.IsSmi() || obj.IsMint()) {
    Integer& integer = Integer::Handle();
    integer ^= obj.raw();
    *value = integer.AsInt64Value();
    return Api::Success();
  }
  if (obj.IsBigint()) {
    Bigint& bigint = Bigint::Handle();
    bigint ^= obj.raw();
    if (BigintOperations::FitsIntoInt64(bigint)) {
      *value = BigintOperations::ToInt64(bigint);
      return Api::Success();
    } else {
      return Api::Error("Integer too big to fit in int64_t");
    }
  }
  return Api::Error("Object is not a Integer");
}


DART_EXPORT Dart_Handle Dart_IntegerValueHexCString(Dart_Handle integer,
                                                    const char** value) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(integer));
  Bigint& bigint = Bigint::Handle();
  if (obj.IsSmi() || obj.IsMint()) {
    Integer& integer = Integer::Handle();
    integer ^= obj.raw();
    bigint ^= BigintOperations::NewFromInt64(integer.AsInt64Value());
    *value = BigintOperations::ToHexCString(bigint, &Api::Allocate);
    return Api::Success();
  }
  if (obj.IsBigint()) {
    bigint ^= obj.raw();
    *value = BigintOperations::ToHexCString(bigint, &Api::Allocate);
    return Api::Success();
  }
  return Api::Error("Object is not a Integer");
}


// --- Booleans ----


DART_EXPORT Dart_Handle Dart_True() {
  CHECK_ISOLATE_SCOPE(Isolate::Current());
  return Api::True();
}


DART_EXPORT Dart_Handle Dart_False() {
  CHECK_ISOLATE_SCOPE(Isolate::Current());
  return Api::False();
}


DART_EXPORT bool Dart_IsBoolean(Dart_Handle object) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(object));
  return obj.IsBool();
}


DART_EXPORT Dart_Handle Dart_NewBoolean(bool value) {
  CHECK_ISOLATE_SCOPE(Isolate::Current());
  return value ? Api::True() : Api::False();
}


DART_EXPORT Dart_Handle Dart_BooleanValue(Dart_Handle bool_object,
                                          bool* value) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(bool_object));
  if (obj.IsBool()) {
    Bool& bool_obj = Bool::Handle();
    bool_obj ^= obj.raw();
    *value = bool_obj.value();
    return Api::Success();
  }
  return Api::Error("Object is not a Boolean");
}


// --- Doubles ---


DART_EXPORT bool Dart_IsDouble(Dart_Handle object) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(object));
  return obj.IsDouble();
}


DART_EXPORT Dart_Handle Dart_NewDouble(double value) {
  DARTSCOPE(Isolate::Current());
  const Double& obj = Double::Handle(Double::New(value));
  return Api::NewLocalHandle(obj);
}


DART_EXPORT Dart_Handle Dart_DoubleValue(Dart_Handle integer, double* result) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(integer));
  if (obj.IsDouble()) {
    Double& double_obj = Double::Handle();
    double_obj ^= obj.raw();
    *result = double_obj.value();
    return Api::Success();
  }
  return Api::Error("Object is not a Double");
}


// --- Strings ---


DART_EXPORT bool Dart_IsString(Dart_Handle object) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(object));
  return obj.IsString();
}


DART_EXPORT bool Dart_IsString8(Dart_Handle object) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(object));
  return obj.IsOneByteString() || obj.IsExternalOneByteString();
}


DART_EXPORT bool Dart_IsString16(Dart_Handle object) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(object));
  return (obj.IsOneByteString() || obj.IsExternalOneByteString() ||
          obj.IsTwoByteString() || obj.IsExternalTwoByteString());
}


DART_EXPORT Dart_Handle Dart_StringLength(Dart_Handle str, intptr_t* len) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(str));
  if (obj.IsString()) {
    String& string_obj = String::Handle();
    string_obj ^= obj.raw();
    *len = string_obj.Length();
    return Api::Success();
  }
  return Api::Error("Object is not a String");
}


DART_EXPORT Dart_Handle Dart_NewString(const char* str) {
  DARTSCOPE(Isolate::Current());
  const String& obj = String::Handle(String::New(str));
  return Api::NewLocalHandle(obj);
}


DART_EXPORT Dart_Handle Dart_NewString8(const uint8_t* codepoints,
                                        intptr_t length) {
  DARTSCOPE(Isolate::Current());
  const String& obj = String::Handle(String::New(codepoints, length));
  return Api::NewLocalHandle(obj);
}


DART_EXPORT Dart_Handle Dart_NewString16(const uint16_t* codepoints,
                                         intptr_t length) {
  DARTSCOPE(Isolate::Current());
  const String& obj = String::Handle(String::New(codepoints, length));
  return Api::NewLocalHandle(obj);
}


DART_EXPORT Dart_Handle Dart_NewString32(const uint32_t* codepoints,
                                         intptr_t length) {
  DARTSCOPE(Isolate::Current());
  const String& obj = String::Handle(String::New(codepoints, length));
  return Api::NewLocalHandle(obj);
}


DART_EXPORT bool Dart_IsExternalString(Dart_Handle object) {
  DARTSCOPE(Isolate::Current());
  const String& str = Api::UnwrapStringHandle(object);
  if (str.IsNull()) {
    return false;
  }
  return str.IsExternal();
}


DART_EXPORT Dart_Handle Dart_ExternalStringGetPeer(Dart_Handle object,
                                                   void** peer) {
  DARTSCOPE(Isolate::Current());
  const String& str = Api::UnwrapStringHandle(object);
  if (str.IsNull()) {
    RETURN_TYPE_ERROR(object, String);
  }
  if (!str.IsExternal()) {
    return Api::Error("%s expects argument 'object' to be an external String.",
                      CURRENT_FUNC);
  }
  if (peer == NULL) {
    return Api::Error("%s expects argument 'peer' to be non-NULL.",
                      CURRENT_FUNC);
  }
  *peer = str.GetPeer();
  return Api::Success();
}


DART_EXPORT Dart_Handle Dart_NewExternalString8(const uint8_t* codepoints,
                                                intptr_t length,
                                                void* peer,
                                                Dart_PeerFinalizer callback) {
  DARTSCOPE(Isolate::Current());
  const String& obj =
      String::Handle(String::NewExternal(codepoints, length, peer, callback));
  return Api::NewLocalHandle(obj);
}


DART_EXPORT Dart_Handle Dart_NewExternalString16(const uint16_t* codepoints,
                                                 intptr_t length,
                                                 void* peer,
                                                 Dart_PeerFinalizer callback) {
  DARTSCOPE(Isolate::Current());
  const String& obj =
      String::Handle(String::NewExternal(codepoints, length, peer, callback));
  return Api::NewLocalHandle(obj);
}


DART_EXPORT Dart_Handle Dart_NewExternalString32(const uint32_t* codepoints,
                                                 intptr_t length,
                                                 void* peer,
                                                 Dart_PeerFinalizer callback) {
  DARTSCOPE(Isolate::Current());
  const String& obj =
      String::Handle(String::NewExternal(codepoints, length, peer, callback));
  return Api::NewLocalHandle(obj);
}


DART_EXPORT Dart_Handle Dart_StringGet8(Dart_Handle str,
                                        uint8_t* codepoints,
                                        intptr_t* length) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(str));
  if (obj.IsString()) {
    String& string_obj = String::Handle();
    string_obj ^= obj.raw();
    if (string_obj.CharSize() == String::kOneByteChar) {
      intptr_t str_len = string_obj.Length();
      intptr_t copy_len = (str_len > *length) ? *length : str_len;
      for (intptr_t i = 0; i < copy_len; i++) {
        codepoints[i] = static_cast<uint8_t>(string_obj.CharAt(i));
      }
      *length= copy_len;
      return Api::Success();
    }
  }
  return Api::Error(obj.IsString()
                    ? "Object is not a String8"
                    : "Object is not a String");
}


DART_EXPORT Dart_Handle Dart_StringGet16(Dart_Handle str,
                                         uint16_t* codepoints,
                                         intptr_t* length) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(str));
  if (obj.IsString()) {
    String& string_obj = String::Handle();
    string_obj ^= obj.raw();
    if (string_obj.CharSize() <= String::kTwoByteChar) {
      intptr_t str_len = string_obj.Length();
      intptr_t copy_len = (str_len > *length) ? *length : str_len;
      for (intptr_t i = 0; i < copy_len; i++) {
        codepoints[i] = static_cast<uint16_t>(string_obj.CharAt(i));
      }
      *length = copy_len;
      return Api::Success();
    }
  }
  return Api::Error(obj.IsString()
                    ? "Object is not a String16"
                    : "Object is not a String");
}


DART_EXPORT Dart_Handle Dart_StringGet32(Dart_Handle str,
                                         uint32_t* codepoints,
                                         intptr_t* length) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(str));
  if (obj.IsString()) {
    String& string_obj = String::Handle();
    string_obj ^= obj.raw();
    intptr_t str_len = string_obj.Length();
    intptr_t copy_len = (str_len > *length) ? *length : str_len;
    for (intptr_t i = 0; i < copy_len; i++) {
      codepoints[i] = static_cast<uint32_t>(string_obj.CharAt(i));
    }
    *length = copy_len;
    return Api::Success();
  }
  return Api::Error("Object is not a String");
}


DART_EXPORT Dart_Handle Dart_StringToCString(Dart_Handle object,
                                             const char** result) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(object));
  if (obj.IsString()) {
    const char* string_value = obj.ToCString();
    intptr_t string_length = strlen(string_value);
    char* res = reinterpret_cast<char*>(Api::Allocate(string_length + 1));
    if (res == NULL) {
      return Api::Error("Unable to allocate memory");
    }
    strncpy(res, string_value, string_length + 1);
    ASSERT(res[string_length] == '\0');
    *result = res;
    return Api::Success();
  }
  return Api::Error("Object is not a String");
}


// --- Lists ---


static RawInstance* GetListInstance(Isolate* isolate, const Object& obj) {
  if (obj.IsInstance()) {
    Instance& instance = Instance::Handle();
    instance ^= obj.raw();
    const Type& type = Type::Handle(isolate->object_store()->list_interface());
    if (instance.IsInstanceOf(type, TypeArguments::Handle())) {
      return instance.raw();
    }
  }
  return Instance::null();
}


DART_EXPORT bool Dart_IsList(Dart_Handle object) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  const Object& obj = Object::Handle(Api::UnwrapHandle(object));
  // TODO(5526318): Make access to GrowableObjectArray more efficient.
  return (obj.IsArray() ||
          (GetListInstance(isolate, obj) != Instance::null()));
}


DART_EXPORT Dart_Handle Dart_NewList(intptr_t length) {
  DARTSCOPE(Isolate::Current());
  const Array& obj = Array::Handle(Array::New(length));
  return Api::NewLocalHandle(obj);
}


DART_EXPORT Dart_Handle Dart_ListLength(Dart_Handle list, intptr_t* len) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  const Object& obj = Object::Handle(Api::UnwrapHandle(list));
  if (obj.IsArray()) {
    Array& array_obj = Array::Handle();
    array_obj ^= obj.raw();
    *len = array_obj.Length();
    return Api::Success();
  }
  // TODO(5526318): Make access to GrowableObjectArray more efficient.
  // Now check and handle a dart object that implements the List interface.
  const Instance& instance = Instance::Handle(GetListInstance(isolate, obj));
  if (!instance.IsNull()) {
    String& name = String::Handle(String::New("length"));
    name = Field::GetterName(name);
    const Function& function = Function::Handle(
        Resolver::ResolveDynamic(instance, name, 1, 0));
    if (!function.IsNull()) {
      GrowableArray<const Object*> args(0);
      LongJump* base = isolate->long_jump_base();
      LongJump jump;
      isolate->set_long_jump_base(&jump);
      Dart_Handle result;
      if (setjmp(*jump.Set()) == 0) {
        const Array& kNoArgumentNames = Array::Handle();
        const Instance& retval = Instance::Handle(
            DartEntry::InvokeDynamic(instance,
                                     function,
                                     args,
                                     kNoArgumentNames));
        result = Api::Success();
        if (retval.IsSmi() || retval.IsMint()) {
          Integer& integer = Integer::Handle();
          integer ^= retval.raw();
          *len = integer.AsInt64Value();
        } else if (retval.IsBigint()) {
          Bigint& bigint = Bigint::Handle();
          bigint ^= retval.raw();
          if (BigintOperations::FitsIntoInt64(bigint)) {
            *len = BigintOperations::ToInt64(bigint);
          } else {
            result = Api::Error("Length of List object is greater than the "
                                "maximum value that 'len' parameter can hold");
          }
        } else if (retval.IsUnhandledException()) {
          result = Api::ErrorFromException(retval);
        } else {
          result = Api::Error("Length of List object is not an integer");
        }
      } else {
        SetupErrorResult(&result);
      }
      isolate->set_long_jump_base(base);
      return result;
    }
  }
  return Api::Error("Object does not implement the list inteface");
}


static RawObject* GetListAt(Isolate* isolate,
                            const Instance& instance,
                            const Integer& index,
                            const Function& function,
                            Dart_Handle* result) {
  ASSERT(isolate != NULL);
  ASSERT(result != NULL);
  LongJump* base = isolate->long_jump_base();
  LongJump jump;
  isolate->set_long_jump_base(&jump);
  if (setjmp(*jump.Set()) == 0) {
    Instance& retval = Instance::Handle();
    GrowableArray<const Object*> args(0);
    args.Add(&index);
    const Array& kNoArgumentNames = Array::Handle();
    retval = DartEntry::InvokeDynamic(instance,
                                      function,
                                      args,
                                      kNoArgumentNames);
    if (retval.IsUnhandledException()) {
      *result = Api::ErrorFromException(retval);
    } else {
      *result = Api::Success();
    }
    isolate->set_long_jump_base(base);
    return retval.raw();
  }
  SetupErrorResult(result);
  isolate->set_long_jump_base(base);
  return Object::null();
}


DART_EXPORT Dart_Handle Dart_ListGetAt(Dart_Handle list, intptr_t index) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  const Object& obj = Object::Handle(Api::UnwrapHandle(list));
  if (obj.IsArray()) {
    Array& array_obj = Array::Handle();
    array_obj ^= obj.raw();
    if ((index >= 0) && (index < array_obj.Length())) {
      const Object& element = Object::Handle(array_obj.At(index));
      return Api::NewLocalHandle(element);
    }
    return Api::Error("Invalid index passed in to access array element");
  }
  // TODO(5526318): Make access to GrowableObjectArray more efficient.
  // Now check and handle a dart object that implements the List interface.
  const Instance& instance = Instance::Handle(GetListInstance(isolate, obj));
  if (!instance.IsNull()) {
    String& name = String::Handle(String::New("[]"));
    const Function& function = Function::Handle(
        Resolver::ResolveDynamic(instance, name, 2, 0));
    if (!function.IsNull()) {
      Object& element = Object::Handle();
      Integer& indexobj = Integer::Handle();
      Dart_Handle result;
      indexobj = Integer::New(index);
      element = GetListAt(isolate, instance, indexobj, function, &result);
      if (::Dart_IsError(result)) {
        return result;  // Error condition.
      }
      return Api::NewLocalHandle(element);
    }
  }
  return Api::Error("Object does not implement the 'List' interface");
}


static void SetListAt(Isolate* isolate,
                      const Instance& instance,
                      const Integer& index,
                      const Object& value,
                      const Function& function,
                      Dart_Handle* result) {
  ASSERT(isolate != NULL);
  ASSERT(result != NULL);
  LongJump* base = isolate->long_jump_base();
  LongJump jump;
  isolate->set_long_jump_base(&jump);
  if (setjmp(*jump.Set()) == 0) {
    GrowableArray<const Object*> args(1);
    args.Add(&index);
    args.Add(&value);
    Instance& retval = Instance::Handle();
    const Array& kNoArgumentNames = Array::Handle();
    retval = DartEntry::InvokeDynamic(instance,
                                      function,
                                      args,
                                      kNoArgumentNames);
    if (retval.IsUnhandledException()) {
      *result = Api::ErrorFromException(retval);
    } else {
      *result = Api::Success();
    }
  } else {
    SetupErrorResult(result);
  }
  isolate->set_long_jump_base(base);
}


DART_EXPORT Dart_Handle Dart_ListSetAt(Dart_Handle list,
                                       intptr_t index,
                                       Dart_Handle value) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  const Object& obj = Object::Handle(Api::UnwrapHandle(list));
  if (obj.IsArray()) {
    if (obj.IsImmutableArray()) {
      return Api::Error("Cannot modify immutable array");
    }
    Array& array_obj = Array::Handle();
    array_obj ^= obj.raw();
    const Object& value_obj = Object::Handle(Api::UnwrapHandle(value));
    if ((index >= 0) && (index < array_obj.Length())) {
      array_obj.SetAt(index, value_obj);
      return Api::Success();
    }
    return Api::Error("Invalid index passed in to set array element");
  }
  // TODO(5526318): Make access to GrowableObjectArray more efficient.
  // Now check and handle a dart object that implements the List interface.
  const Instance& instance = Instance::Handle(GetListInstance(isolate, obj));
  if (!instance.IsNull()) {
    String& name = String::Handle(String::New("[]="));
    const Function& function = Function::Handle(
        Resolver::ResolveDynamic(instance, name, 3, 0));
    if (!function.IsNull()) {
      Dart_Handle result;
      const Integer& index_obj = Integer::Handle(Integer::New(index));
      const Object& value_obj = Object::Handle(Api::UnwrapHandle(value));
      SetListAt(isolate, instance, index_obj, value_obj, function, &result);
      return result;
    }
  }
  return Api::Error("Object does not implement the 'List' interface");
}


DART_EXPORT Dart_Handle Dart_ListGetAsBytes(Dart_Handle list,
                                            intptr_t offset,
                                            uint8_t* native_array,
                                            intptr_t length) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  const Object& obj = Object::Handle(Api::UnwrapHandle(list));
  if (obj.IsArray()) {
    Array& array_obj = Array::Handle();
    array_obj ^= obj.raw();
    if ((offset + length) <= array_obj.Length()) {
      Object& element = Object::Handle();
      Integer& integer  = Integer::Handle();
      for (int i = 0; i < length; i++) {
        element = array_obj.At(offset + i);
        if (!element.IsInteger()) {
          return Api::Error("%s expects the argument 'list' to be "
                            "a List of int", CURRENT_FUNC);
        }
        integer ^= element.raw();
        native_array[i] = static_cast<uint8_t>(integer.AsInt64Value() & 0xff);
        ASSERT(integer.AsInt64Value() <= 0xff);
        // TODO(hpayer): value should always be smaller then 0xff. Add error
        // handling.
      }
      return Api::Success();
    }
    return Api::Error("Invalid length passed in to access array elements");
  }
  // TODO(5526318): Make access to GrowableObjectArray more efficient.
  // Now check and handle a dart object that implements the List interface.
  const Instance& instance = Instance::Handle(GetListInstance(isolate, obj));
  if (!instance.IsNull()) {
    String& name = String::Handle(String::New("[]"));
    const Function& function = Function::Handle(
        Resolver::ResolveDynamic(instance, name, 2, 0));
    if (!function.IsNull()) {
      Object& element = Object::Handle();
      Integer& intobj = Integer::Handle();
      Dart_Handle result;
      for (int i = 0; i < length; i++) {
        intobj = Integer::New(offset + i);
        element = GetListAt(isolate, instance, intobj, function, &result);
        if (::Dart_IsError(result)) {
          return result;  // Error condition.
        }
        if (!element.IsInteger()) {
          return Api::Error("%s expects the argument 'list' to be "
                            "a List of int", CURRENT_FUNC);
        }
        intobj ^= element.raw();
        ASSERT(intobj.AsInt64Value() <= 0xff);
        // TODO(hpayer): value should always be smaller then 0xff. Add error
        // handling.
        native_array[i] = static_cast<uint8_t>(intobj.AsInt64Value() & 0xff);
      }
      return Api::Success();
    }
  }
  return Api::Error("Object does not implement the 'List' interface");
}


DART_EXPORT Dart_Handle Dart_ListSetAsBytes(Dart_Handle list,
                                            intptr_t offset,
                                            uint8_t* native_array,
                                            intptr_t length) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  const Object& obj = Object::Handle(Api::UnwrapHandle(list));
  if (obj.IsArray()) {
    if (obj.IsImmutableArray()) {
      return Api::Error("Cannot modify immutable array");
    }
    Array& array_obj = Array::Handle();
    array_obj ^= obj.raw();
    Integer& integer = Integer::Handle();
    if ((offset + length) <= array_obj.Length()) {
      for (int i = 0; i < length; i++) {
        integer = Integer::New(native_array[i]);
        array_obj.SetAt(offset + i, integer);
      }
      return Api::Success();
    }
    return Api::Error("Invalid length passed in to set array elements");
  }
  // TODO(5526318): Make access to GrowableObjectArray more efficient.
  // Now check and handle a dart object that implements the List interface.
  const Instance& instance = Instance::Handle(GetListInstance(isolate, obj));
  if (!instance.IsNull()) {
    String& name = String::Handle(String::New("[]="));
    const Function& function = Function::Handle(
        Resolver::ResolveDynamic(instance, name, 3, 0));
    if (!function.IsNull()) {
      Integer& indexobj = Integer::Handle();
      Integer& valueobj = Integer::Handle();
      Dart_Handle result;
      for (int i = 0; i < length; i++) {
        indexobj = Integer::New(offset + i);
        valueobj = Integer::New(native_array[i]);
        SetListAt(isolate, instance, indexobj, valueobj, function, &result);
        if (::Dart_IsError(result)) {
          return result;  // Error condition.
        }
      }
      return Api::Success();
    }
  }
  return Api::Error("Object does not implement the 'List' interface");
}


// --- Closures ---


DART_EXPORT bool Dart_IsClosure(Dart_Handle object) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(object));
  return obj.IsClosure();
}


// NOTE: Need to pass 'result' as a parameter here in order to avoid
// warning: variable 'result' might be clobbered by 'longjmp' or 'vfork'
// which shows up because of the use of setjmp.
static void InvokeClosure(Isolate* isolate,
                          const Closure& closure,
                          GrowableArray<const Object*>& args,
                          Dart_Handle* result) {
  ASSERT(isolate != NULL);
  LongJump* base = isolate->long_jump_base();
  LongJump jump;
  isolate->set_long_jump_base(&jump);
  if (setjmp(*jump.Set()) == 0) {
    const Array& kNoArgumentNames = Array::Handle();
    const Instance& retval = Instance::Handle(
        DartEntry::InvokeClosure(closure, args, kNoArgumentNames));
    if (retval.IsUnhandledException()) {
      *result = Api::ErrorFromException(retval);
    } else {
      *result = Api::NewLocalHandle(retval);
    }
  } else {
    SetupErrorResult(result);
  }
  isolate->set_long_jump_base(base);
}


DART_EXPORT Dart_Handle Dart_InvokeClosure(Dart_Handle closure,
                                           int number_of_arguments,
                                           Dart_Handle* arguments) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  const Object& obj = Object::Handle(Api::UnwrapHandle(closure));
  if (obj.IsNull()) {
    return Api::Error("Null object passed in to invoke closure");
  }
  if (!obj.IsClosure()) {
    return Api::Error("Invalid closure passed to invoke closure");
  }
  ASSERT(ClassFinalizer::AllClassesFinalized());

  // Now try to invoke the closure.
  Closure& closure_obj = Closure::Handle();
  closure_obj ^= obj.raw();
  Dart_Handle retval;
  GrowableArray<const Object*> dart_arguments(number_of_arguments);
  for (int i = 0; i < number_of_arguments; i++) {
    const Object& arg = Object::Handle(Api::UnwrapHandle(arguments[i]));
    dart_arguments.Add(&arg);
  }
  InvokeClosure(isolate, closure_obj, dart_arguments, &retval);
  return retval;
}


DART_EXPORT int64_t Dart_ClosureSmrck(Dart_Handle object) {
  DARTSCOPE(Isolate::Current());
  const Closure& obj = Closure::CheckedHandle(Api::UnwrapHandle(object));
  const Integer& smrck = Integer::Handle(obj.smrck());
  return smrck.IsNull() ? 0 : smrck.AsInt64Value();
}


DART_EXPORT void Dart_ClosureSetSmrck(Dart_Handle object, int64_t value) {
  DARTSCOPE(Isolate::Current());
  const Closure& obj = Closure::CheckedHandle(Api::UnwrapHandle(object));
  const Integer& smrck = Integer::Handle(Integer::New(value));
  obj.set_smrck(smrck);
}


// --- Methods and Fields ---


DART_EXPORT Dart_Handle Dart_InvokeStatic(Dart_Handle library_in,
                                          Dart_Handle class_name_in,
                                          Dart_Handle function_name_in,
                                          int number_of_arguments,
                                          Dart_Handle* arguments) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  // Finalize all classes.
  const char* msg = CheckIsolateState(isolate);
  if (msg != NULL) {
    return Api::Error(msg);
  }

  // Now try to resolve and invoke the static function.
  const Library& library =
      Library::CheckedHandle(Api::UnwrapHandle(library_in));
  if (library.IsNull()) {
    return Api::Error("No library specified");
  }
  const String& class_name =
      String::CheckedHandle(Api::UnwrapHandle(class_name_in));
  const String& function_name =
      String::CheckedHandle(Api::UnwrapHandle(function_name_in));
  const Function& function = Function::Handle(
      Resolver::ResolveStatic(library,
                              class_name,
                              function_name,
                              number_of_arguments,
                              Array::Handle(),  // Named arguments are not yet
                                                // supported in the API.
                              Resolver::kIsQualified));
  if (function.IsNull()) {
    char* msg;
    if (class_name.IsNull()) {
      const char* format = "Unable to find entrypoint: %s()";
      intptr_t length = OS::SNPrint(NULL, 0, format, function_name.ToCString());
      msg = reinterpret_cast<char*>(Api::Allocate(length + 1));
      OS::SNPrint(msg, (length + 1), format, function_name.ToCString());
    } else {
      const char* format = "Unable to find entrypoint: static %s.%s()";
      intptr_t length = OS::SNPrint(NULL, 0, format,
                                    class_name.ToCString(),
                                    function_name.ToCString());
      msg = reinterpret_cast<char*>(Api::Allocate(length + 1));
      OS::SNPrint(msg, (length + 1), format,
                  class_name.ToCString(), function_name.ToCString());
    }
    return Api::Error(msg);
  }
  Dart_Handle retval;
  GrowableArray<const Object*> dart_arguments(number_of_arguments);
  for (int i = 0; i < number_of_arguments; i++) {
    const Object& arg = Object::Handle(Api::UnwrapHandle(arguments[i]));
    dart_arguments.Add(&arg);
  }
  InvokeStatic(isolate, function, dart_arguments, &retval);
  return retval;
}


DART_EXPORT Dart_Handle Dart_InvokeDynamic(Dart_Handle object,
                                           Dart_Handle function_name,
                                           int number_of_arguments,
                                           Dart_Handle* arguments) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  const Object& obj = Object::Handle(Api::UnwrapHandle(object));
  // Let the resolver figure out the correct target for null receiver.
  // E.g., (null).toString() should execute correctly.
  if (!obj.IsNull() && !obj.IsInstance()) {
    return Api::Error(
        "Invalid receiver (not instance) passed to invoke dynamic");
  }
  if (function_name == NULL) {
    return Api::Error("Invalid function name specified");
  }
  ASSERT(ClassFinalizer::AllClassesFinalized());

  // Now try to resolve and invoke the dynamic function on this object.
  Instance& receiver = Instance::Handle();
  receiver ^= obj.raw();
  const String& name = String::CheckedHandle(Api::UnwrapHandle(function_name));
  const Function& function = Function::Handle(
      Resolver::ResolveDynamic(receiver,
                               name,
                               (number_of_arguments + 1),
                               0));  // Named args not yet supported in API.
  if (function.IsNull()) {
    // TODO(5415268): Invoke noSuchMethod instead of failing.
    OS::PrintErr("Unable to find instance function: %s\n", name.ToCString());
    return Api::Error("Unable to find instance function");
  }
  Dart_Handle retval;
  GrowableArray<const Object*> dart_arguments(number_of_arguments);
  for (int i = 0; i < number_of_arguments; i++) {
    const Object& arg = Object::Handle(Api::UnwrapHandle(arguments[i]));
    dart_arguments.Add(&arg);
  }
  InvokeDynamic(isolate, receiver, function, dart_arguments, &retval);
  return retval;
}


static const bool kGetter = true;
static const bool kSetter = false;


static bool UseGetterForStaticField(const Field& fld) {
  if (fld.IsNull()) {
    return true;
  }

  // Return getter method for uninitialized fields, rather than the
  // field object, since the value in the field object will not be
  // initialized until the first time the getter is invoked.
  const Instance& value = Instance::Handle(fld.value());
  ASSERT(value.raw() != Object::transition_sentinel());
  return value.raw() == Object::sentinel();
}


static Dart_Handle LookupStaticField(Dart_Handle clazz,
                                     Dart_Handle field_name,
                                     bool is_getter) {
  const Object& param1 = Object::Handle(Api::UnwrapHandle(clazz));
  const Object& param2 = Object::Handle(Api::UnwrapHandle(field_name));
  if (param1.IsNull() || !param1.IsClass()) {
    return Api::Error("Invalid class specified");
  }
  if (param2.IsNull() || !param2.IsString()) {
    return Api::Error("Invalid field name specified");
  }
  Class& cls = Class::Handle();
  cls ^= param1.raw();
  String& fld_name = String::Handle();
  fld_name ^= param2.raw();
  const Field& fld = Field::Handle(cls.LookupStaticField(fld_name));
  if (is_getter && UseGetterForStaticField(fld)) {
    const String& func_name = String::Handle(Field::GetterName(fld_name));
    const Function& function =
        Function::Handle(cls.LookupStaticFunction(func_name));
    if (!function.IsNull()) {
      return Api::NewLocalHandle(function);
    }
    return Api::Error("Specified field is not found in the class");
  }
  if (fld.IsNull()) {
    return Api::Error("Specified field is not found in the class");
  }
  return Api::NewLocalHandle(fld);
}


static Dart_Handle LookupInstanceField(const Object& object,
                                       Dart_Handle name,
                                       bool is_getter) {
  const Object& param = Object::Handle(Api::UnwrapHandle(name));
  if (param.IsNull() || !param.IsString()) {
    return Api::Error("Invalid field name specified");
  }
  String& field_name = String::Handle();
  field_name ^= param.raw();
  String& func_name = String::Handle();
  Field& fld = Field::Handle();
  Class& cls = Class::Handle(object.clazz());
  while (!cls.IsNull()) {
    fld = cls.LookupInstanceField(field_name);
    if (!fld.IsNull()) {
      if (!is_getter && fld.is_final()) {
        return Api::Error("Cannot set value of final fields");
      }
      func_name = (is_getter
                   ? Field::GetterName(field_name)
                   : Field::SetterName(field_name));
      const Function& function = Function::Handle(
          cls.LookupDynamicFunction(func_name));
      if (function.IsNull()) {
        return Api::Error("Unable to find accessor function in the class");
      }
      return Api::NewLocalHandle(function);
    }
    cls = cls.SuperClass();
  }
  return Api::Error("Unable to find field in the class");
}


DART_EXPORT Dart_Handle Dart_GetStaticField(Dart_Handle cls,
                                            Dart_Handle name) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  Dart_Handle result = LookupStaticField(cls, name, kGetter);
  if (::Dart_IsError(result)) {
    return result;
  }
  Object& retval = Object::Handle();
  const Object& obj = Object::Handle(Api::UnwrapHandle(result));
  if (obj.IsField()) {
    Field& fld = Field::Handle();
    fld ^= obj.raw();
    retval = fld.value();
    return Api::NewLocalHandle(retval);
  } else {
    Function& func = Function::Handle();
    func ^= obj.raw();
    GrowableArray<const Object*> args;
    InvokeStatic(isolate, func, args, &result);
    return result;
  }
}


// TODO(iposva): The value parameter should be documented as being an instance.
// TODO(turnidge): Is this skipping the setter?
DART_EXPORT Dart_Handle Dart_SetStaticField(Dart_Handle cls,
                                            Dart_Handle name,
                                            Dart_Handle value) {
  DARTSCOPE(Isolate::Current());
  Dart_Handle result = LookupStaticField(cls, name, kSetter);
  if (::Dart_IsError(result)) {
    return result;
  }
  Field& fld = Field::Handle();
  fld ^= Api::UnwrapHandle(result);
  if (fld.is_final()) {
    return Api::Error("Specified field is a static final field in the class");
  }
  const Object& val = Object::Handle(Api::UnwrapHandle(value));
  Instance& instance = Instance::Handle();
  instance ^= val.raw();
  fld.set_value(instance);
  return Api::NewLocalHandle(val);
}


DART_EXPORT Dart_Handle Dart_GetInstanceField(Dart_Handle obj,
                                              Dart_Handle name) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  const Object& param = Object::Handle(Api::UnwrapHandle(obj));
  if (param.IsNull() || !param.IsInstance()) {
    return Api::Error("Invalid object passed in to access instance field");
  }
  Instance& object = Instance::Handle();
  object ^= param.raw();
  Dart_Handle result = LookupInstanceField(object, name, kGetter);
  if (::Dart_IsError(result)) {
    return result;
  }
  Function& func = Function::Handle();
  func ^= Api::UnwrapHandle(result);
  GrowableArray<const Object*> arguments;
  InvokeDynamic(isolate, object, func, arguments, &result);
  return result;
}


DART_EXPORT Dart_Handle Dart_SetInstanceField(Dart_Handle obj,
                                              Dart_Handle name,
                                              Dart_Handle value) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  const Object& param = Object::Handle(Api::UnwrapHandle(obj));
  if (param.IsNull() || !param.IsInstance()) {
    return Api::Error("Invalid object passed in to access instance field");
  }
  Instance& object = Instance::Handle();
  object ^= param.raw();
  Dart_Handle result = LookupInstanceField(object, name, kSetter);
  if (::Dart_IsError(result)) {
    return result;
  }
  Function& func = Function::Handle();
  func ^= Api::UnwrapHandle(result);
  GrowableArray<const Object*> arguments(1);
  const Object& arg = Object::Handle(Api::UnwrapHandle(value));
  arguments.Add(&arg);
  InvokeDynamic(isolate, object, func, arguments, &result);
  return result;
}


DART_EXPORT Dart_Handle Dart_CreateNativeWrapperClass(Dart_Handle library,
                                                      Dart_Handle name,
                                                      int field_count) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  const Object& param = Object::Handle(Api::UnwrapHandle(name));
  if (param.IsNull() || !param.IsString() || field_count <= 0) {
    return Api::Error(
        "Invalid arguments passed to Dart_CreateNativeWrapperClass");
  }
  String& cls_name = String::Handle();
  cls_name ^= param.raw();
  cls_name = String::NewSymbol(cls_name);
  Library& lib = Library::Handle();
  lib ^= Api::UnwrapHandle(library);
  if (lib.IsNull()) {
    return Api::Error(
        "Invalid arguments passed to Dart_CreateNativeWrapperClass");
  }
  const Class& cls = Class::Handle(Class::NewNativeWrapper(&lib,
                                                           cls_name,
                                                           field_count));
  if (cls.IsNull()) {
    return Api::Error(
        "Unable to create native wrapper class : already exists");
  }
  return Api::NewLocalHandle(cls);
}


DART_EXPORT Dart_Handle Dart_GetNativeInstanceField(Dart_Handle obj,
                                                    int index,
                                                    intptr_t* value) {
  DARTSCOPE(Isolate::Current());
  const Object& param = Object::Handle(Api::UnwrapHandle(obj));
  if (param.IsNull() || !param.IsInstance()) {
    return Api::Error(
        "Invalid object passed in to access native instance field");
  }
  Instance& object = Instance::Handle();
  object ^= param.raw();
  if (!object.IsValidNativeIndex(index)) {
    return Api::Error(
        "Invalid index passed in to access native instance field");
  }
  *value = object.GetNativeField(index);
  return Api::Success();
}


DART_EXPORT Dart_Handle Dart_SetNativeInstanceField(Dart_Handle obj,
                                                    int index,
                                                    intptr_t value) {
  DARTSCOPE(Isolate::Current());
  const Object& param = Object::Handle(Api::UnwrapHandle(obj));
  if (param.IsNull() || !param.IsInstance()) {
    return Api::Error("Invalid object passed in to set native instance field");
  }
  Instance& object = Instance::Handle();
  object ^= param.raw();
  if (!object.IsValidNativeIndex(index)) {
    return Api::Error("Invalid index passed in to set native instance field");
  }
  object.SetNativeField(index, value);
  return Api::Success();
}


// --- Exceptions ----


DART_EXPORT Dart_Handle Dart_ThrowException(Dart_Handle exception) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  if (isolate->top_exit_frame_info() == 0) {
    // There are no dart frames on the stack so it would be illegal to
    // throw an exception here.
    return Api::Error("No Dart frames on stack, cannot throw exception");
  }
  const Instance& excp = Instance::CheckedHandle(Api::UnwrapHandle(exception));
  // Unwind all the API scopes till the exit frame before throwing an
  // exception.
  ApiState* state = isolate->api_state();
  ASSERT(state != NULL);
  state->UnwindScopes(isolate->top_exit_frame_info());
  Exceptions::Throw(excp);
  return Api::Error("Exception was not thrown, internal error");
}


DART_EXPORT Dart_Handle Dart_ReThrowException(Dart_Handle exception,
                                              Dart_Handle stacktrace) {
  Isolate* isolate = Isolate::Current();
  CHECK_ISOLATE(isolate);
  if (isolate->top_exit_frame_info() == 0) {
    // There are no dart frames on the stack so it would be illegal to
    // throw an exception here.
    return Api::Error("No Dart frames on stack, cannot throw exception");
  }
  DARTSCOPE(isolate);
  const Instance& excp = Instance::CheckedHandle(Api::UnwrapHandle(exception));
  const Instance& stk = Instance::CheckedHandle(Api::UnwrapHandle(stacktrace));
  // Unwind all the API scopes till the exit frame before throwing an
  // exception.
  ApiState* state = isolate->api_state();
  ASSERT(state != NULL);
  state->UnwindScopes(isolate->top_exit_frame_info());
  Exceptions::ReThrow(excp, stk);
  return Api::Error("Exception was not re thrown, internal error");
}


// --- Native functions ---


DART_EXPORT Dart_Handle Dart_GetNativeArgument(Dart_NativeArguments args,
                                               int index) {
  DARTSCOPE(Isolate::Current());
  NativeArguments* arguments = reinterpret_cast<NativeArguments*>(args);
  const Object& obj = Object::Handle(arguments->At(index));
  return Api::NewLocalHandle(obj);
}


DART_EXPORT int Dart_GetNativeArgumentCount(Dart_NativeArguments args) {
  CHECK_ISOLATE(Isolate::Current());
  NativeArguments* arguments = reinterpret_cast<NativeArguments*>(args);
  return arguments->Count();
}


DART_EXPORT void Dart_SetReturnValue(Dart_NativeArguments args,
                                     Dart_Handle retval) {
  DARTSCOPE(Isolate::Current());
  NativeArguments* arguments = reinterpret_cast<NativeArguments*>(args);
  arguments->SetReturn(Object::Handle(Api::UnwrapHandle(retval)));
}


// --- Scripts and Libraries ---


// NOTE: Need to pass 'result' as a parameter here in order to avoid
// warning: variable 'result' might be clobbered by 'longjmp' or 'vfork'
// which shows up because of the use of setjmp.
static void CompileSource(Isolate* isolate,
                          const Library& lib,
                          const String& url,
                          const String& source,
                          RawScript::Kind kind,
                          Dart_Handle* result) {
  bool update_lib_status = (kind == RawScript::kScript ||
                            kind == RawScript::kLibrary);
  if (update_lib_status) {
    lib.SetLoadInProgress();
  }
  const Script& script = Script::Handle(Script::New(url, source, kind));
  ASSERT(isolate != NULL);
  LongJump* base = isolate->long_jump_base();
  LongJump jump;
  isolate->set_long_jump_base(&jump);
  if (setjmp(*jump.Set()) == 0) {
    Compiler::Compile(lib, script);
    *result = Api::NewLocalHandle(lib);
    if (update_lib_status) {
      lib.SetLoaded();
    }
  } else {
    SetupErrorResult(result);
    if (update_lib_status) {
      lib.SetLoadError();
    }
  }
  isolate->set_long_jump_base(base);
}


DART_EXPORT Dart_Handle Dart_LoadScript(Dart_Handle url,
                                        Dart_Handle source,
                                        Dart_LibraryTagHandler handler) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  TIMERSCOPE(time_script_loading);
  const String& url_str = Api::UnwrapStringHandle(url);
  if (url_str.IsNull()) {
    RETURN_TYPE_ERROR(url, String);
  }
  const String& source_str = Api::UnwrapStringHandle(source);
  if (source_str.IsNull()) {
    RETURN_TYPE_ERROR(source, String);
  }
  Library& library = Library::Handle(isolate->object_store()->root_library());
  if (!library.IsNull()) {
    const String& library_url = String::Handle(library.url());
    return Api::Error("%s: A script has already been loaded from '%s'.",
                      CURRENT_FUNC, library_url.ToCString());
  }
  isolate->set_library_tag_handler(handler);
  library = Library::New(url_str);
  library.Register();
  isolate->object_store()->set_root_library(library);
  Dart_Handle result;
  CompileSource(isolate,
                library,
                url_str,
                source_str,
                RawScript::kScript,
                &result);
  return result;
}


DART_EXPORT Dart_Handle Dart_LoadScriptFromSnapshot(const uint8_t* buffer) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  if (buffer == NULL) {
    return Api::Error("%s expects argument 'buffer' to be non-null.",
                      CURRENT_FUNC);
  }
  const Snapshot* snapshot = Snapshot::SetupFromBuffer(buffer);
  if (!snapshot->IsScriptSnapshot()) {
    return Api::Error("%s expects parameter 'buffer' to be a script type"
                      " snapshot", CURRENT_FUNC);
  }
  Library& library = Library::Handle(isolate->object_store()->root_library());
  if (!library.IsNull()) {
    const String& library_url = String::Handle(library.url());
    return Api::Error("%s: A script has already been loaded from '%s'.",
                      CURRENT_FUNC, library_url.ToCString());
  }
  SnapshotReader reader(snapshot, isolate->heap(), isolate->object_store());
  const Object& tmp = Object::Handle(reader.ReadObject());
  if (!tmp.IsLibrary()) {
    return Api::Error("%s: Unable to deserialize snapshot correctly.",
                      CURRENT_FUNC);
  }
  library ^= tmp.raw();
  isolate->object_store()->set_root_library(library);
  return Api::NewLocalHandle(library);
}


DEFINE_FLAG(bool, compile_all, false, "Eagerly compile all code.");

static void CompileAll(Isolate* isolate, Dart_Handle* result) {
  *result = Api::Success();
  if (FLAG_compile_all) {
    ASSERT(isolate != NULL);
    LongJump* base = isolate->long_jump_base();
    LongJump jump;
    isolate->set_long_jump_base(&jump);
    if (setjmp(*jump.Set()) == 0) {
      Library::CompileAll();
    } else {
      SetupErrorResult(result);
    }
    isolate->set_long_jump_base(base);
  }
}


DART_EXPORT Dart_Handle Dart_CompileAll() {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  Dart_Handle result;
  const char* msg = CheckIsolateState(isolate);
  if (msg != NULL) {
    return Api::Error(msg);
  }
  CompileAll(isolate, &result);
  return result;
}


DART_EXPORT bool Dart_IsLibrary(Dart_Handle object) {
  DARTSCOPE(Isolate::Current());
  const Object& obj = Object::Handle(Api::UnwrapHandle(object));
  return obj.IsLibrary();
}


DART_EXPORT Dart_Handle Dart_GetClass(Dart_Handle library, Dart_Handle name) {
  DARTSCOPE(Isolate::Current());
  const Object& param = Object::Handle(Api::UnwrapHandle(name));
  if (param.IsNull() || !param.IsString()) {
    return Api::Error("Invalid class name specified");
  }
  const Library& lib = Library::CheckedHandle(Api::UnwrapHandle(library));
  if (lib.IsNull()) {
    return Api::Error("Invalid parameter, Unknown library specified");
  }
  String& cls_name = String::Handle();
  cls_name ^= param.raw();
  const Class& cls = Class::Handle(lib.LookupClass(cls_name));
  if (cls.IsNull()) {
    const String& lib_name = String::Handle(lib.name());
    return Api::Error("Class '%s' not found in library '%s'.",
                      cls_name.ToCString(), lib_name.ToCString());
  }
  return Api::NewLocalHandle(cls);
}


DART_EXPORT Dart_Handle Dart_LibraryUrl(Dart_Handle library) {
  DARTSCOPE(Isolate::Current());
  const Library& lib = Api::UnwrapLibraryHandle(library);
  if (lib.IsNull()) {
    RETURN_TYPE_ERROR(library, Library);
  }
  const String& url = String::Handle(lib.url());
  ASSERT(!url.IsNull());
  return Api::NewLocalHandle(url);
}


DART_EXPORT Dart_Handle Dart_LookupLibrary(Dart_Handle url) {
  DARTSCOPE(Isolate::Current());
  const String& url_str = Api::UnwrapStringHandle(url);
  if (url_str.IsNull()) {
    RETURN_TYPE_ERROR(url, String);
  }
  const Library& library = Library::Handle(Library::LookupLibrary(url_str));
  if (library.IsNull()) {
    return Api::Error("%s: library '%s' not found.",
                      CURRENT_FUNC, url_str.ToCString());
  } else {
    return Api::NewLocalHandle(library);
  }
}


DART_EXPORT Dart_Handle Dart_LoadLibrary(Dart_Handle url, Dart_Handle source) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  const String& url_str = Api::UnwrapStringHandle(url);
  if (url_str.IsNull()) {
    RETURN_TYPE_ERROR(url, String);
  }
  const String& source_str = Api::UnwrapStringHandle(source);
  if (source_str.IsNull()) {
    RETURN_TYPE_ERROR(source, String);
  }
  Library& library = Library::Handle(Library::LookupLibrary(url_str));
  if (library.IsNull()) {
    library = Library::New(url_str);
    library.Register();
  } else if (!library.LoadNotStarted()) {
    // The source for this library has either been loaded or is in the
    // process of loading.  Return an error.
    return Api::Error("%s: library '%s' has already been loaded.",
                      CURRENT_FUNC, url_str.ToCString());
  }
  Dart_Handle result;
  CompileSource(isolate,
                library,
                url_str,
                source_str,
                RawScript::kLibrary,
                &result);
  return result;
}


DART_EXPORT Dart_Handle Dart_LibraryImportLibrary(Dart_Handle library,
                                                  Dart_Handle import) {
  DARTSCOPE(Isolate::Current());
  const Library& library_vm = Api::UnwrapLibraryHandle(library);
  if (library_vm.IsNull()) {
    RETURN_TYPE_ERROR(library, Library);
  }
  const Library& import_vm = Api::UnwrapLibraryHandle(import);
  if (import_vm.IsNull()) {
    RETURN_TYPE_ERROR(import, Library);
  }
  library_vm.AddImport(import_vm);
  return Api::Success();
}


DART_EXPORT Dart_Handle Dart_LoadSource(Dart_Handle library,
                                        Dart_Handle url,
                                        Dart_Handle source) {
  Isolate* isolate = Isolate::Current();
  DARTSCOPE(isolate);
  const Library& lib = Api::UnwrapLibraryHandle(library);
  if (lib.IsNull()) {
    RETURN_TYPE_ERROR(library, Library);
  }
  const String& url_str = Api::UnwrapStringHandle(url);
  if (url_str.IsNull()) {
    RETURN_TYPE_ERROR(url, String);
  }
  const String& source_str = Api::UnwrapStringHandle(source);
  if (source_str.IsNull()) {
    RETURN_TYPE_ERROR(source, String);
  }
  Dart_Handle result;
  CompileSource(isolate, lib, url_str, source_str, RawScript::kSource, &result);
  return result;
}


DART_EXPORT Dart_Handle Dart_SetNativeResolver(
    Dart_Handle library,
    Dart_NativeEntryResolver resolver) {
  DARTSCOPE(Isolate::Current());
  const Library& lib = Api::UnwrapLibraryHandle(library);
  if (lib.IsNull()) {
    RETURN_TYPE_ERROR(library, Library);
  }
  lib.set_native_entry_resolver(resolver);
  return Api::Success();
}


// --- Profiling support ----


DART_EXPORT void Dart_InitPprofSupport() {
  DebugInfo* pprof_symbol_generator = DebugInfo::NewGenerator();
  ASSERT(pprof_symbol_generator != NULL);
  Dart::set_pprof_symbol_generator(pprof_symbol_generator);
}


DART_EXPORT void Dart_GetPprofSymbolInfo(void** buffer, int* buffer_size) {
  DebugInfo* pprof_symbol_generator = Dart::pprof_symbol_generator();
  if (pprof_symbol_generator != NULL) {
    ByteArray* debug_region = new ByteArray();
    ASSERT(debug_region != NULL);
    pprof_symbol_generator->WriteToMemory(debug_region);
    *buffer_size = debug_region->size();
    if (*buffer_size != 0) {
      *buffer = reinterpret_cast<void*>(Api::Allocate(*buffer_size));
      memmove(*buffer, debug_region->data(), *buffer_size);
    } else {
      *buffer = NULL;
    }
    delete debug_region;
  } else {
    *buffer = NULL;
    *buffer_size = 0;
  }
}


}  // namespace dart
