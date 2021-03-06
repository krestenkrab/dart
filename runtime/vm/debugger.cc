// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/debugger.h"

#include "vm/code_index_table.h"
#include "vm/code_patcher.h"
#include "vm/compiler.h"
#include "vm/flags.h"
#include "vm/globals.h"
#include "vm/object.h"
#include "vm/object_store.h"
#include "vm/os.h"
#include "vm/stack_frame.h"
#include "vm/stub_code.h"
#include "vm/visitor.h"


namespace dart {


DEFINE_FLAG(charp, bpt, NULL, "Debug breakpoint at <func>");


Breakpoint::Breakpoint(const Function& func, intptr_t pc_desc_index)
    : function_(func.raw()),
      pc_desc_index_(pc_desc_index),
      pc_(0),
      line_number_(-1),
      next_(NULL) {
  Code& code = Code::Handle(func.code());
  ASSERT(!code.IsNull());  // Function must be compiled.
  PcDescriptors& desc = PcDescriptors::Handle(code.pc_descriptors());
  ASSERT(pc_desc_index < desc.Length());
  this->token_index_ = desc.TokenIndex(pc_desc_index);
  ASSERT(this->token_index_ > 0);
  this->pc_ = desc.PC(pc_desc_index);
  ASSERT(this->pc_ != 0);
}


RawScript* Breakpoint::SourceCode() {
  const Function& func = Function::Handle(this->function_);
  const Class& cls = Class::Handle(func.owner());
  return cls.script();
}


RawString* Breakpoint::SourceUrl() {
  const Script& script = Script::Handle(this->SourceCode());
  return script.url();
}


intptr_t Breakpoint::LineNumber() {
  // Compute line number lazily since it causes scanning of the script.
  if (this->line_number_ < 0) {
    const Script& script = Script::Handle(this->SourceCode());
    intptr_t ignore_column;
    script.GetTokenLocation(this->token_index_,
                            &this->line_number_, &ignore_column);
  }
  return this->line_number_;
}


void Breakpoint::VisitObjectPointers(ObjectPointerVisitor* visitor) {
  visitor->VisitPointer(reinterpret_cast<RawObject**>(&function_));
}


ActivationFrame::ActivationFrame(uword pc)
    : pc_(pc),
      function_(Function::null()),
      token_index_(-1),
      line_number_(-1) {
}


RawFunction* ActivationFrame::DartFunction() {
  if (function_ == Function::null()) {
    ASSERT(Isolate::Current() != NULL);
    CodeIndexTable* code_index_table = Isolate::Current()->code_index_table();
    ASSERT(code_index_table != NULL);
    function_ = code_index_table->LookupFunction(pc_);
  }
  return function_;
}


RawString* ActivationFrame::SourceUrl() {
  const Script& script = Script::Handle(SourceScript());
  return script.url();
}


RawScript* ActivationFrame::SourceScript() {
  const Function& func = Function::Handle(DartFunction());
  const Class& cls = Class::Handle(func.owner());
  return cls.script();
}


intptr_t ActivationFrame::TokenIndex() {
  if (token_index_ < 0) {
    const Function& func = Function::Handle(DartFunction());
    Code& code = Code::Handle(func.code());
    ASSERT(!code.IsNull());
    PcDescriptors& desc = PcDescriptors::Handle(code.pc_descriptors());
    for (int i = 0; i < desc.Length(); i++) {
      if (desc.PC(i) == pc_) {
        token_index_ = desc.TokenIndex(i);
        break;
      }
    }
    ASSERT(token_index_ >= 0);
  }
  return token_index_;
}


intptr_t ActivationFrame::LineNumber() {
  // Compute line number lazily since it causes scanning of the script.
  if (line_number_ < 0) {
    const Script& script = Script::Handle(SourceScript());
    intptr_t ignore_column;
    script.GetTokenLocation(TokenIndex(), &line_number_, &ignore_column);
  }
  return line_number_;
}


RawArray* ActivationFrame::Variables() {
  UNIMPLEMENTED();
  return NULL;
}


RawInstance* ActivationFrame::Value(const String& variable_name) {
  UNIMPLEMENTED();
  return NULL;
}


const char* ActivationFrame::ToCString() {
  const char* kFormat = "Function: '%s%s%s' url: '%s' line: %d";

  Function& func = Function::Handle(DartFunction());
  String& func_name = String::Handle(func.name());
  Class& func_class = Class::Handle(func.owner());
  String& class_name = String::Handle(func_class.Name());
  String& url = String::Handle(SourceUrl());
  intptr_t line = LineNumber();

  intptr_t len = OS::SNPrint(NULL, 0, kFormat,
                             class_name.ToCString(),
                             func_class.IsTopLevel() ? "" : ".",
                             func_name.ToCString(),
                             url.ToCString(),
                             line);
  len++;  // String terminator.
  char* chars = reinterpret_cast<char*>(
      Isolate::Current()->current_zone()->Allocate(len));
  OS::SNPrint(chars, len, kFormat,
              class_name.ToCString(),
              func_class.IsTopLevel() ? "" : ".",
              func_name.ToCString(),
              url.ToCString(),
              line);
  return chars;
}


void StackTrace::AddActivation(ActivationFrame* frame) {
  this->trace_.Add(frame);
}


Debugger::Debugger()
    : initialized_(false),
      bp_handler_(NULL),
      breakpoints_(NULL) {
}


bool Debugger::IsActive() {
  // TODO(hausner): The code generator uses this function to prevent
  // generation of optimized code when Dart code is being debugged.
  // This is probably not conservative enough (we could set the first
  // breakpoint after optimized code has already been produced).
  // Long-term, we need to be able to de-optimize code.
  return breakpoints_ != NULL;
}


static RawFunction* ResolveLibraryFunction(
                        const Library& library,
                        const String& fname) {
  ASSERT(!library.IsNull());
  Function& function = Function::Handle();
  const Object& object = Object::Handle(library.LookupObject(fname));
  if (!object.IsNull() && object.IsFunction()) {
    function ^= object.raw();
  }
  return function.raw();
}


RawFunction* Debugger::ResolveFunction(const Library& library,
                                       const String& class_name,
                                       const String& function_name) {
  ASSERT(!library.IsNull());
  ASSERT(!class_name.IsNull());
  ASSERT(!function_name.IsNull());
  if (class_name.Length() == 0) {
    return ResolveLibraryFunction(library, function_name);
  }
  const Class& cls = Class::Handle(library.LookupClass(class_name));
  Function& function = Function::Handle();
  if (!cls.IsNull()) {
    function = cls.LookupStaticFunction(function_name);
    if (function.IsNull()) {
      function = cls.LookupDynamicFunction(function_name);
    }
  }
  return function.raw();
}


Breakpoint* Debugger::SetBreakpointAtEntry(const Function& target_function) {
  ASSERT(!target_function.IsNull());
  if (!target_function.HasCode()) {
    Compiler::CompileFunction(target_function);
  }
  Code& code = Code::Handle(target_function.code());
  ASSERT(!code.IsNull());
  PcDescriptors& desc = PcDescriptors::Handle(code.pc_descriptors());
  for (int i = 0; i < desc.Length(); i++) {
    PcDescriptors::Kind kind = desc.DescriptorKind(i);
    Breakpoint* bpt = NULL;
    if (kind == PcDescriptors::kIcCall) {
      CodePatcher::PatchInstanceCallAt(
          desc.PC(i), StubCode::BreakpointDynamicEntryPoint());
      bpt = new Breakpoint(target_function, i);
    } else if (kind == PcDescriptors::kOther) {
      if (CodePatcher::IsDartCall(desc.PC(i))) {
        CodePatcher::PatchStaticCallAt(
            desc.PC(i), StubCode::BreakpointStaticEntryPoint());
        bpt = new Breakpoint(target_function, i);
      }
    }
    if (bpt != NULL) {
      OS::Print("Setting breakpoint at '%s' line %d  (PC %p)\n",
          String::Handle(bpt->SourceUrl()).ToCString(),
          bpt->LineNumber(),
          bpt->pc());
      AddBreakpoint(bpt);
      return bpt;
    }
  }
  return NULL;
}


void Debugger::VisitObjectPointers(ObjectPointerVisitor* visitor) {
  ASSERT(visitor != NULL);
  Breakpoint* bpt = this->breakpoints_;
  while (bpt != NULL) {
    bpt->VisitObjectPointers(visitor);
    bpt = bpt->next();
  }
}


static void DefaultBreakpointHandler(Breakpoint* bpt, StackTrace* stack) {
  for (intptr_t i = 0; i < stack->Length(); i++) {
    OS::Print("   %d. %s\n",
              i + 1, stack->ActivationFrameAt(i)->ToCString());
  }
}


void Debugger::SetBreakpointHandler(BreakpointHandler* handler) {
  bp_handler_ = handler;
  if (bp_handler_ == NULL) {
    bp_handler_ = &DefaultBreakpointHandler;
  }
}


void Debugger::BreakpointCallback() {
  ASSERT(initialized_);
  DartFrameIterator iterator;
  DartFrame* frame = iterator.NextFrame();
  ASSERT(frame != NULL);
  Breakpoint* bpt = GetBreakpoint(frame->pc());
  ASSERT(bpt != NULL);
  OS::Print(">>> Breakpoint at %s:%d (Address %p)\n",
      bpt ? String::Handle(bpt->SourceUrl()).ToCString() : "?",
      bpt ? bpt->LineNumber() : 0,
      frame->pc());
  StackTrace* stack_trace = new StackTrace(8);
  while (frame != NULL) {
    ASSERT(frame->IsValid());
    ASSERT(frame->IsDartFrame());
    ActivationFrame* activation = new ActivationFrame(frame->pc());
    stack_trace->AddActivation(activation);
    frame = iterator.NextFrame();
  }

  if (bp_handler_ != NULL) {
    (*bp_handler_)(bpt, stack_trace);
  }
}


void Debugger::Initialize(Isolate* isolate) {
  if (initialized_) {
    return;
  }
  initialized_ = true;
  SetBreakpointHandler(DefaultBreakpointHandler);
}


Breakpoint* Debugger::GetBreakpoint(uword breakpoint_address) {
  Breakpoint* bpt = this->breakpoints_;
  while (bpt != NULL) {
    if (bpt->pc() == breakpoint_address) {
      return bpt;
    }
    bpt = bpt->next();
  }
  return NULL;
}


void Debugger::AddBreakpoint(Breakpoint* bpt) {
  ASSERT(bpt->next() == NULL);
  bpt->set_next(this->breakpoints_);
  this->breakpoints_ = bpt;
}


}  // namespace dart
