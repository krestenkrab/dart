// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/globals.h"  // Needed here to get TARGET_ARCH_IA32.
#if defined(TARGET_ARCH_IA32)

#include "vm/code_generator.h"

#include "lib/error.h"
#include "vm/ast_printer.h"
#include "vm/class_finalizer.h"
#include "vm/dart_entry.h"
#include "vm/debugger.h"
#include "vm/ic_data.h"
#include "vm/longjump.h"
#include "vm/object.h"
#include "vm/object_store.h"
#include "vm/parser.h"
#include "vm/resolver.h"
#include "vm/stub_code.h"

namespace dart {

DEFINE_FLAG(bool, print_ast, false, "Print abstract syntax tree.");
DEFINE_FLAG(bool, print_scopes, false, "Print scopes of local variables.");
DEFINE_FLAG(bool, trace_functions, false, "Trace entry of each function.");
DEFINE_FLAG(int, optimization_invocation_threshold, 1000,
    "number of invocations before a function is optimized, -1 means never.");
DECLARE_FLAG(bool, enable_type_checks);
DECLARE_FLAG(bool, report_invocation_count);
DECLARE_FLAG(bool, trace_compiler);

#define __ assembler_->


CodeGeneratorState::CodeGeneratorState(CodeGenerator* codegen)
    : StackResource(Isolate::Current()),
      codegen_(codegen),
      parent_(codegen->state()) {
  if (parent_ != NULL) {
    root_node_ = parent_->root_node_;
    loop_level_ = parent_->loop_level_;
    context_level_ = parent_->context_level_;
    current_try_index_ = parent_->current_try_index_;
  } else {
    root_node_ = NULL;
    loop_level_ = 0;
    context_level_ = 0;
    current_try_index_ = CatchClauseNode::kInvalidTryIndex;
  }
  codegen_->set_state(this);
}


CodeGeneratorState::~CodeGeneratorState() {
  codegen_->set_state(parent_);
}


class CodeGenerator::DescriptorList : public ZoneAllocated {
 public:
  struct PcDesc {
    intptr_t pc_offset;        // PC offset value of the descriptor.
    PcDescriptors::Kind kind;  // Descriptor kind (kDeopt, kOther).
    intptr_t node_id;          // AST node id.
    intptr_t token_index;      // Token position in source of PC.
    intptr_t try_index;        // Try block index of PC.
  };

  DescriptorList() : list_() {
  }
  ~DescriptorList() { }

  intptr_t Length() const {
    return list_.length();
  }

  intptr_t PcOffset(int index) const {
    return list_[index].pc_offset;
  }
  PcDescriptors::Kind Kind(int index) const {
    return list_[index].kind;
  }
  intptr_t NodeId(int index) const {
    return list_[index].node_id;
  }
  intptr_t TokenIndex(int index) const {
    return list_[index].token_index;
  }
  intptr_t TryIndex(int index) const {
    return list_[index].try_index;
  }

  void AddDescriptor(PcDescriptors::Kind kind,
                     intptr_t pc_offset,
                     intptr_t node_id,
                     intptr_t token_index,
                     intptr_t try_index) {
    struct PcDesc data;
    data.pc_offset = pc_offset;
    data.kind = kind;
    data.node_id = node_id;
    data.token_index = token_index;
    data.try_index = try_index;
    list_.Add(data);
  }

  RawPcDescriptors* FinalizePcDescriptors(uword entry_point) {
    intptr_t num_descriptors = Length();
    const PcDescriptors& descriptors =
        PcDescriptors::Handle(PcDescriptors::New(num_descriptors));
    for (intptr_t i = 0; i < num_descriptors; i++) {
      descriptors.AddDescriptor(i,
                                (entry_point + PcOffset(i)),
                                Kind(i),
                                NodeId(i),
                                TokenIndex(i),
                                TryIndex(i));
    }
    return descriptors.raw();
  }

 private:
  GrowableArray<struct PcDesc> list_;
  DISALLOW_COPY_AND_ASSIGN(DescriptorList);
};


class CodeGenerator::HandlerList : public ZoneAllocated {
 public:
  struct HandlerDesc {
    intptr_t try_index;  // Try block index handled by the handler.
    intptr_t pc_offset;  // Handler PC offset value.
  };

  HandlerList() : list_() {
  }
  ~HandlerList() { }

  intptr_t Length() const {
    return list_.length();
  }

  intptr_t TryIndex(int index) const {
    return list_[index].try_index;
  }
  intptr_t PcOffset(int index) const {
    return list_[index].pc_offset;
  }
  void SetPcOffset(int index, intptr_t handler_pc) {
    list_[index].pc_offset = handler_pc;
  }

  void AddHandler(intptr_t try_index, intptr_t pc_offset) {
    struct HandlerDesc data;
    data.try_index = try_index;
    data.pc_offset = pc_offset;
    list_.Add(data);
  }

  RawExceptionHandlers* FinalizeExceptionHandlers(uword entry_point) {
    intptr_t num_handlers = Length();
    const ExceptionHandlers& handlers =
        ExceptionHandlers::Handle(ExceptionHandlers::New(num_handlers));
    for (intptr_t i = 0; i < num_handlers; i++) {
      handlers.SetHandlerEntry(i, TryIndex(i), (entry_point + PcOffset(i)));
    }
    return handlers.raw();
  }

 private:
  GrowableArray<struct HandlerDesc> list_;
  DISALLOW_COPY_AND_ASSIGN(HandlerList);
};


CodeGenerator::CodeGenerator(Assembler* assembler,
                             const ParsedFunction& parsed_function)
    : assembler_(assembler),
      parsed_function_(parsed_function),
      locals_space_size_(-1),
      state_(NULL),
      pc_descriptors_list_(NULL),
      exception_handlers_list_(NULL),
      try_index_(CatchClauseNode::kInvalidTryIndex) {
  ASSERT(assembler_ != NULL);
  ASSERT(parsed_function.node_sequence() != NULL);
  pc_descriptors_list_ = new CodeGenerator::DescriptorList();
  exception_handlers_list_ = new CodeGenerator::HandlerList();
}


bool CodeGenerator::IsResultNeeded(AstNode* node) const {
  return !state()->IsRootNode(node);
}


// NOTE: First 5 bytes of the code may be patched with a jump instruction. Do
// not emit any objects in the first 5 bytes.
void CodeGenerator::GenerateCode() {
  CodeGeneratorState codegen_state(this);
  if (FLAG_print_scopes && FLAG_print_ast) {
    // Print the function scope before code generation.
    AstPrinter::PrintFunctionScope(parsed_function_);
  }
  if (FLAG_print_ast) {
    // Print the function ast before code generation.
    AstPrinter::PrintFunctionNodes(parsed_function_);
  }
  if (FLAG_trace_functions) {
    // Preserve ECX (ic-data array or object) and EDX (arguments descriptor).
    __ pushl(ECX);
    __ pushl(EDX);
    const Function& function =
        Function::ZoneHandle(parsed_function_.function().raw());
    __ LoadObject(EAX, function);
    __ pushl(EAX);
    GenerateCallRuntime(AstNode::kNoId,
                        0,
                        kTraceFunctionEntryRuntimeEntry);
    __ popl(EAX);
    __ popl(EDX);
    __ popl(ECX);
  }

  const bool code_generation_finished = TryIntrinsify();
  // In some cases intrinsifier can generate all code and no AST based
  // code generation is needed. In some cases slow-paths (e.g., overflows) are
  // implemented by the AST based code generation and 'code_generation_finished'
  // is false.
  if (!code_generation_finished) {
    GeneratePreEntryCode();
    GenerateEntryCode();
    if (FLAG_print_scopes) {
      // Print the function scope (again) after generating the prologue in order
      // to see annotations such as allocation indices of locals.
      if (FLAG_print_ast) {
        // Second printing.
        OS::Print("Annotated ");
      }
      AstPrinter::PrintFunctionScope(parsed_function_);
    }
    parsed_function_.node_sequence()->Visit(this);
  }
  // End of code.
  __ int3();
  GenerateDeferredCode();

  // Emit function patching code. This will be swapped with the first 5 bytes
  // at entry point.
  pc_descriptors_list_->AddDescriptor(PcDescriptors::kPatchCode,
                                      assembler_->CodeSize(),
                                      AstNode::kNoId,
                                      0,
                                      -1);
  __ jmp(&StubCode::FixCallersTargetLabel());
}


void CodeGenerator::GenerateDeferredCode() {
}


// Pre entry code is called before the frame has been constructed:
// - check for stack overflow.
// - optionally count function invocations.
// - optionally trigger optimizing compiler if invocation threshold has been
//   reached.
// Note that first 5 bytes may be patched with a jump.
// TODO(srdjan): Add check that no object is inlined in the first
// 5 bytes (length of a jump instruction).
void CodeGenerator::GeneratePreEntryCode() {
  // Stack overflow check.
  __ cmpl(ESP,
      Address::Absolute(Isolate::Current()->stack_limit_address()));
  __ j(BELOW_EQUAL, &StubCode::StackOverflowLabel());
  // Do not optimize if:
  // - we count invocations.
  // - optimization disabled via negative 'optimization_invocation_threshold;
  // - function is marked as non-optimizable.
  // - type checks are enabled.
  const bool may_optimize =
      !FLAG_report_invocation_count &&
      (FLAG_optimization_invocation_threshold >= 0) &&
      !Isolate::Current()->debugger()->IsActive() &&
      parsed_function_.function().is_optimizable();
  // Count invocation and check.
  if (FLAG_report_invocation_count || may_optimize) {
    const Function& function =
        Function::ZoneHandle(parsed_function_.function().raw());
    __ LoadObject(EAX, function);
    __ movl(EBX, FieldAddress(EAX, Function::invocation_counter_offset()));
    __ incl(EBX);
    if (may_optimize) {
      __ cmpl(EBX, Immediate(FLAG_optimization_invocation_threshold));
      __ j(GREATER, &StubCode::OptimizeInvokedFunctionLabel());
    }
    __ movl(FieldAddress(EAX, Function::invocation_counter_offset()), EBX);
  }
}


// Verify assumptions (in debug mode only).
// - No two deopt descriptors have the same node id (deoptimization).
// - No two ic-call descriptors have the same node id (type feedback).
// - No two descriptors of same kind have the same PC.
// A function without unique ids is marked as non-optimizable (e.g., because of
// finally blocks).
static void VerifyPcDescriptors(const PcDescriptors& descriptors,
                                bool check_ids) {
#if defined(DEBUG)
  // TODO(srdjan): Implement a more efficient way to check, currently drop
  // the check for too large number of descriptors.
  if (descriptors.Length() > 3000) {
    if (FLAG_trace_compiler) {
      OS::Print("Not checking pc decriptors, length %d\n",
                descriptors.Length());
    }
    return;
  }
  for (intptr_t i = 0; i < descriptors.Length(); i++) {
    uword pc = descriptors.PC(i);
    PcDescriptors::Kind kind = descriptors.DescriptorKind(i);
    // 'node_id' is set for kDeopt and kIcCall and must be unique for one kind.
    intptr_t node_id = AstNode::kNoId;
    if (check_ids) {
      if ((descriptors.DescriptorKind(i) == PcDescriptors::kDeopt) ||
          (descriptors.DescriptorKind(i) == PcDescriptors::kIcCall)) {
        node_id = descriptors.NodeId(i);
      }
    }
    for (intptr_t k = i + 1; k < descriptors.Length(); k++) {
      if (kind == descriptors.DescriptorKind(k)) {
        if (node_id != AstNode::kNoId) {
          ASSERT(descriptors.NodeId(k) != node_id);
        }
        ASSERT(pc != descriptors.PC(k));
      }
    }
  }
#endif  // DEBUG
}


void CodeGenerator::FinalizePcDescriptors(const Code& code) {
  ASSERT(pc_descriptors_list_ != NULL);
  const PcDescriptors& descriptors = PcDescriptors::Handle(
      pc_descriptors_list_->FinalizePcDescriptors(code.EntryPoint()));
  VerifyPcDescriptors(
      descriptors, parsed_function_.function().is_optimizable());
  code.set_pc_descriptors(descriptors);
}


void CodeGenerator::FinalizeExceptionHandlers(const Code& code) {
  ASSERT(exception_handlers_list_ != NULL);
  const ExceptionHandlers& handlers = ExceptionHandlers::Handle(
      exception_handlers_list_->FinalizeExceptionHandlers(code.EntryPoint()));
  code.set_exception_handlers(handlers);
}


void CodeGenerator::GenerateLoadVariable(Register dst,
                                         const LocalVariable& variable) {
  if (variable.is_captured()) {
    // The variable lives in the context.
    int delta = state()->context_level() - variable.owner()->context_level();
    ASSERT(delta >= 0);
    Register base = CTX;
    while (delta-- > 0) {
      __ movl(dst, FieldAddress(base, Context::parent_offset()));
      base = dst;
    }
    __ movl(dst,
            FieldAddress(base, Context::variable_offset(variable.index())));
  } else {
    // The variable lives in the current stack frame.
    __ movl(dst, Address(EBP, variable.index() * kWordSize));
  }
}


void CodeGenerator::GenerateStoreVariable(const LocalVariable& variable,
                                          Register src,
                                          Register scratch) {
  if (variable.is_captured()) {
    // The variable lives in the context.
    int delta = state()->context_level() - variable.owner()->context_level();
    ASSERT(delta >= 0);
    Register base = CTX;
    while (delta-- > 0) {
      __ movl(scratch, FieldAddress(base, Context::parent_offset()));
      base = scratch;
    }
    __ movl(FieldAddress(base, Context::variable_offset(variable.index())),
            src);
  } else {
    // The variable lives in the current stack frame.
    __ movl(Address(EBP, variable.index() * kWordSize), src);
  }
}


void CodeGenerator::GeneratePushVariable(const LocalVariable& variable,
                                         Register scratch) {
  if (variable.is_captured()) {
    // The variable lives in the context.
    int delta = state()->context_level() - variable.owner()->context_level();
    ASSERT(delta >= 0);
    Register base = CTX;
    while (delta-- > 0) {
      __ movl(scratch, FieldAddress(base, Context::parent_offset()));
      base = scratch;
    }
    __ pushl(FieldAddress(base, Context::variable_offset(variable.index())));
  } else {
    // The variable lives in the current stack frame.
    __ pushl(Address(EBP, variable.index() * kWordSize));
  }
}


void CodeGenerator::GenerateInstanceCall(
    intptr_t node_id,
    intptr_t token_index,
    const String& function_name,
    int num_arguments,
    const Array& optional_arguments_names) {
  // Set up the function name and number of arguments (including the receiver)
  // to the InstanceCall stub which will resolve the correct entrypoint for
  // the operator and call it.
  ICData ic_data(function_name, 1);
  __ LoadObject(ECX, Array::ZoneHandle(ic_data.data()));
  __ LoadObject(EDX, ArgumentsDescriptor(num_arguments,
                                         optional_arguments_names));
  ExternalLabel target_label(
      "InlineCache", StubCode::InlineCacheEntryPoint());

  __ call(&target_label);
  AddCurrentDescriptor(PcDescriptors::kIcCall,
                       node_id,
                       token_index);
  __ addl(ESP, Immediate(num_arguments * kWordSize));
}


// Call to generate entry code:
// - compute frame size and setup frame.
// - allocate local variables on stack.
// - optionally check if number of arguments match.
// - initialize all non-argument locals to null.
//
// Input parameters:
//   ESP : points to return address.
//   ESP + 4 : address of last argument (arg n-1).
//   ESP + 4*n : address of first argument (arg 0).
//   EDX : arguments descriptor array.
void CodeGenerator::GenerateEntryCode() {
  const Immediate raw_null =
      Immediate(reinterpret_cast<intptr_t>(Object::null()));
  const Function& function = parsed_function_.function();
  LocalScope* scope = parsed_function_.node_sequence()->scope();
  const int num_fixed_params = function.num_fixed_parameters();
  const int num_opt_params = function.num_optional_parameters();
  const int num_params = num_fixed_params + num_opt_params;
  int first_param_index;
  int first_local_index;
  int num_copied_params;
  // Assign indices to parameters and locals.
  if (num_params == num_fixed_params) {
    // No need to copy incoming arguments.
    // The body of the function will access parameter i at fp[1 + num_fixed - i]
    // and local variable j at fp[-1 - j].
    first_param_index = 1 + num_params;
    first_local_index = -1;
    num_copied_params = 0;
  } else {
    // The body of the function will access copied parameter i at fp[-1 - i]
    // and local j at fp[-1 - num_params - j].
    first_param_index = -1;
    first_local_index = -1 - num_params;
    num_copied_params = num_params;
    ASSERT(num_copied_params > 0);
  }

  // Allocate parameters and local variables, either in the local frame or in
  // the context(s).
  LocalScope* context_owner = NULL;  // No context needed so far.
  int first_free_frame_index =
      scope->AllocateVariables(first_param_index,
                               num_params,
                               first_local_index,
                               scope,  // Initial loop owner.
                               &context_owner);
  // Frame indices are relative to the frame pointer and are decreasing.
  ASSERT(first_free_frame_index <= first_local_index);
  const int num_locals = first_local_index - first_free_frame_index;

  // Reserve local space for copied incoming and default arguments and locals.
  // TODO(regis): We may give up reserving space on stack for args/locals
  // because pushes of initial values may be more effective than moves.
  set_locals_space_size((num_copied_params + num_locals) * kWordSize);
  __ EnterFrame(locals_space_size());

  // We check the number of passed arguments when we have to copy them due to
  // the presence of optional named parameters.
  // No such checking code is generated if only fixed parameters are declared,
  // unless we are debug mode or unless we are compiling a closure.
  if (num_copied_params == 0) {
#if defined(DEBUG)
    const bool check_arguments = true;  // Always check arguments in debug mode.
#else
    // The number of arguments passed to closure functions must always be
    // checked here, because no resolving stub (normally responsible for the
    // check) is involved in closure calls.
    const bool check_arguments = function.IsClosureFunction();
#endif
    if (check_arguments) {
      // Check that num_fixed <= argc <= num_params.
      Label argc_in_range;
      // Total number of args is the first Smi in args descriptor array (EDX).
      __ movl(EAX, FieldAddress(EDX, Array::data_offset()));
      if (num_opt_params == 0) {
        __ cmpl(EAX, Immediate(Smi::RawValue(num_fixed_params)));
        __ j(EQUAL, &argc_in_range, Assembler::kNearJump);
      } else {
        __ subl(EAX, Immediate(Smi::RawValue(num_fixed_params)));
        __ cmpl(EAX, Immediate(Smi::RawValue(num_opt_params)));
        __ j(BELOW_EQUAL, &argc_in_range, Assembler::kNearJump);
      }
      if (function.IsClosureFunction()) {
        GenerateCallRuntime(AstNode::kNoId,
                            function.token_index(),
                            kClosureArgumentMismatchRuntimeEntry);
      } else {
        __ Stop("Wrong number of arguments");
      }
      __ Bind(&argc_in_range);
    }
  } else {
    ASSERT(first_param_index == -1);
    // Copy positional arguments.
    // Check that no fewer than num_fixed_params positional arguments are passed
    // in and that no more than num_params arguments are passed in.
    // Passed argument i at fp[1 + argc - i] copied to fp[-1 - i].

    // Total number of args is the first Smi in args descriptor array (EDX).
    __ movl(EBX, FieldAddress(EDX, Array::data_offset()));
    // Check that num_args <= num_params.
    Label wrong_num_arguments;
    __ cmpl(EBX, Immediate(Smi::RawValue(num_params)));
    __ j(GREATER, &wrong_num_arguments);
    // Number of positional args is the second Smi in descriptor array (EDX).
    __ movl(ECX, FieldAddress(EDX, Array::data_offset() + (1 * kWordSize)));
    // Check that num_pos_args >= num_fixed_params.
    __ cmpl(ECX, Immediate(Smi::RawValue(num_fixed_params)));
    __ j(LESS, &wrong_num_arguments);
    // Since EBX and ECX are Smi, use TIMES_2 instead of TIMES_4.
    // Let EBX point to the last passed positional argument, i.e. to
    // fp[1 + num_args - (num_pos_args - 1)].
    __ subl(EBX, ECX);
    __ leal(EBX, Address(EBP, EBX, TIMES_2, 2 * kWordSize));
    // Let EDI point to the last copied positional argument, i.e. to
    // fp[-1 - (num_pos_args - 1)].
    __ movl(EDI, EBP);
    __ subl(EDI, ECX);  // ECX is a Smi, subtract twice for TIMES_4 scaling.
    __ subl(EDI, ECX);
    __ SmiUntag(ECX);
    Label loop, loop_condition;
    __ jmp(&loop_condition, Assembler::kNearJump);
    // We do not use the final allocation index of the variable here, i.e.
    // scope->VariableAt(i)->index(), because captured variables still need
    // to be copied to the context that is not yet allocated.
    const Address argument_addr(EBX, ECX, TIMES_4, 0);
    const Address copy_addr(EDI, ECX, TIMES_4, 0);
    __ Bind(&loop);
    __ movl(EAX, argument_addr);
    __ movl(copy_addr, EAX);
    __ Bind(&loop_condition);
    __ decl(ECX);
    __ j(POSITIVE, &loop, Assembler::kNearJump);

    // Copy or initialize optional named arguments.
    ASSERT(num_opt_params > 0);  // Or we would not have to copy arguments.
    // Start by alphabetically sorting the names of the optional parameters.
    LocalVariable** opt_param = new LocalVariable*[num_opt_params];
    int* opt_param_position = new int[num_opt_params];
    for (int pos = num_fixed_params; pos < num_params; pos++) {
      LocalVariable* parameter = scope->VariableAt(pos);
      const String& opt_param_name = parameter->name();
      int i = pos - num_fixed_params;
      while (--i >= 0) {
        LocalVariable* param_i = opt_param[i];
        const intptr_t result = opt_param_name.CompareTo(param_i->name());
        ASSERT(result != 0);
        if (result > 0) break;
        opt_param[i + 1] = opt_param[i];
        opt_param_position[i + 1] = opt_param_position[i];
      }
      opt_param[i + 1] = parameter;
      opt_param_position[i + 1] = pos;
    }
    // Generate code handling each optional parameter in alphabetical order.
    // Total number of args is the first Smi in args descriptor array (EDX).
    __ movl(EBX, FieldAddress(EDX, Array::data_offset()));
    // Number of positional args is the second Smi in descriptor array (EDX).
    __ movl(ECX, FieldAddress(EDX, Array::data_offset() + (1 * kWordSize)));
    __ SmiUntag(ECX);
    // Let EBX point to the first passed argument, i.e. to fp[1 + argc - 0].
    __ leal(EBX, Address(EBP, EBX, TIMES_2, kWordSize));
    // Let EDI point to the name/pos pair of the first named argument.
    __ leal(EDI, FieldAddress(EDX, Array::data_offset() + (2 * kWordSize)));
    for (int i = 0; i < num_opt_params; i++) {
      // Handle this optional parameter only if k or fewer positional arguments
      // have been passed, where k is the position of this optional parameter in
      // the formal parameter list.
      Label load_default_value, assign_optional_parameter, next_parameter;
      const int param_pos = opt_param_position[i];
      __ cmpl(ECX, Immediate(param_pos));
      __ j(GREATER, &next_parameter, Assembler::kNearJump);
      // Check if this named parameter was passed in.
      __ movl(EAX, Address(EDI, 0));  // Load EAX with the name of the argument.
      __ CompareObject(EAX, opt_param[i]->name());
      __ j(NOT_EQUAL, &load_default_value, Assembler::kNearJump);
      // Load EAX with passed-in argument at provided arg_pos, i.e. at
      // fp[1 + argc - arg_pos].
      __ movl(EAX, Address(EDI, kWordSize));  // EAX is arg_pos as Smi.
      __ addl(EDI, Immediate(2 * kWordSize));  // Point to next name/pos pair.
      __ negl(EAX);
      Address argument_addr(EBX, EAX, TIMES_2, 0);  // EAX is a negative Smi.
      __ movl(EAX, argument_addr);
      __ jmp(&assign_optional_parameter, Assembler::kNearJump);
      __ Bind(&load_default_value);
      // Load EAX with default argument at pos.
      const Object& value = Object::ZoneHandle(
          parsed_function_.default_parameter_values().At(
              param_pos - num_fixed_params));
      __ LoadObject(EAX, value);
      __ Bind(&assign_optional_parameter);
      // Assign EAX to fp[-1 - param_pos].
      // We do not use the final allocation index of the variable here, i.e.
      // scope->VariableAt(i)->index(), because captured variables still need
      // to be copied to the context that is not yet allocated.
      const Address param_addr(EBP, (-1 - param_pos) * kWordSize);
      __ movl(param_addr, EAX);
      __ Bind(&next_parameter);
    }
    delete[] opt_param;
    delete[] opt_param_position;
    // Check that EDI now points to the null terminator in the array descriptor.
    Label all_arguments_processed;
    __ cmpl(Address(EDI, 0), raw_null);
    __ j(EQUAL, &all_arguments_processed, Assembler::kNearJump);

    __ Bind(&wrong_num_arguments);
    if (function.IsClosureFunction()) {
      GenerateCallRuntime(AstNode::kNoId,
                          function.token_index(),
                          kClosureArgumentMismatchRuntimeEntry);
    } else {
      // Invoke noSuchMethod function.
      ICData ic_data(String::Handle(function.name()), 1);
      __ LoadObject(ECX, Array::ZoneHandle(ic_data.data()));
      // EBP : points to previous frame pointer.
      // EBP + 4 : points to return address.
      // EBP + 8 : address of last argument (arg n-1).
      // ESP + 8 + 4*(n-1) : address of first argument (arg 0).
      // ECX : ic-data array.
      // EDX : arguments descriptor array.
      __ call(&StubCode::CallNoSuchMethodFunctionLabel());
    }

    if (FLAG_trace_functions) {
      __ pushl(EAX);  // Preserve result.
      __ PushObject(function);
      GenerateCallRuntime(AstNode::kNoId,
                          0,
                          kTraceFunctionExitRuntimeEntry);
      __ popl(EAX);  // Remove argument.
      __ popl(EAX);  // Restore result.
    }
    __ LeaveFrame();
    __ ret();

    __ Bind(&all_arguments_processed);
    // Nullify originally passed arguments only after they have been copied and
    // checked, otherwise noSuchMethod would not see their original values.
    // This step can be skipped in case we decide that formal parameters are
    // implicitly final, since garbage collecting the unmodified value is not
    // an issue anymore.

    // EDX : arguments descriptor array.
    // Total number of args is the first Smi in args descriptor array (EDX).
    __ movl(ECX, FieldAddress(EDX, Array::data_offset()));
    __ SmiUntag(ECX);
    Label null_args_loop, null_args_loop_condition;
    __ jmp(&null_args_loop_condition, Assembler::kNearJump);
    const Address original_argument_addr(EBP, ECX, TIMES_4, 2 * kWordSize);
    __ Bind(&null_args_loop);
    __ movl(original_argument_addr, raw_null);
    __ Bind(&null_args_loop_condition);
    __ decl(ECX);
    __ j(POSITIVE, &null_args_loop, Assembler::kNearJump);
  }

  // Initialize locals.
  // TODO(regis): For now, always unroll the init loop. Decide later above
  // which threshold to implement a loop.
  // Consider emitting pushes instead of moves.
  for (int index = first_local_index; index > first_free_frame_index; index--) {
    if (index == first_local_index) {
      __ movl(EAX, raw_null);
    }
    __ movl(Address(EBP, index * kWordSize), EAX);
  }
}


void CodeGenerator::GenerateReturnEpilog() {
  // Unchain the context(s) up to context level 0.
  int context_level = state()->context_level();
  ASSERT(context_level >= 0);
  while (context_level-- > 0) {
    __ movl(CTX, FieldAddress(CTX, Context::parent_offset()));
  }
#ifdef DEBUG
  // Check that the entry stack size matches the exit stack size.
  __ movl(EDX, EBP);
  __ subl(EDX, ESP);
  ASSERT(locals_space_size() >= 0);
  __ cmpl(EDX, Immediate(locals_space_size()));
  Label wrong_stack;
  __ j(NOT_EQUAL, &wrong_stack, Assembler::kNearJump);
#endif  // DEBUG.

  if (FLAG_trace_functions) {
    __ pushl(EAX);  // Preserve result.
    const Function& function =
        Function::ZoneHandle(parsed_function_.function().raw());
    __ LoadObject(EBX, function);
    __ pushl(EBX);
    GenerateCallRuntime(AstNode::kNoId,
                        0,
                        kTraceFunctionExitRuntimeEntry);
    __ popl(EAX);  // Remove argument.
    __ popl(EAX);  // Restore result.
  }
  __ LeaveFrame();
  __ ret();

#ifdef DEBUG
  __ Bind(&wrong_stack);
  __ Stop("Exit stack size does not match the entry stack size.");
#endif  // DEBUG.
}


void CodeGenerator::VisitReturnNode(ReturnNode* node) {
  ASSERT(!IsResultNeeded(node));
  ASSERT(node->value() != NULL);

  if (!node->value()->IsLiteralNode()) {
    node->value()->Visit(this);
    // The result of the return value is now on top of the stack.
  }

  // Generate inlined code for all finally blocks as we are about to transfer
  // control out of the 'try' blocks if any.
  for (intptr_t i = 0; i < node->inlined_finally_list_length(); i++) {
    node->InlinedFinallyNodeAt(i)->Visit(this);
  }

  if (node->value()->IsLiteralNode()) {
    // Load literal value into EAX.
    const Object& literal = node->value()->AsLiteralNode()->literal();
    if (literal.IsSmi()) {
      __ movl(EAX, Immediate(reinterpret_cast<int32_t>(literal.raw())));
    } else {
      __ LoadObject(EAX, literal);
    }
  } else {
    // Pop the previously evaluated result value into EAX.
    __ popl(EAX);
  }

  // Generate type check.
  if (FLAG_enable_type_checks) {
    const RawFunction::Kind  kind = parsed_function().function().kind();
    // Implicit getters do not need a type check at return.
    if ((kind != RawFunction::kImplicitGetter) &&
        (kind != RawFunction::kConstImplicitGetter)) {
      GenerateAssertAssignable(
          node->id(),
          node->value()->token_index(),
          AbstractType::ZoneHandle(parsed_function().function().result_type()),
          String::ZoneHandle(String::NewSymbol("function result")));
    }
  }
  GenerateReturnEpilog();
}


void CodeGenerator::VisitLiteralNode(LiteralNode* node) {
  if (!IsResultNeeded(node)) return;
  __ PushObject(node->literal());
}


void CodeGenerator::VisitTypeNode(TypeNode* node) {
  // Type nodes are handled specially by the code generator.
  UNREACHABLE();
}


void CodeGenerator::VisitAssignableNode(AssignableNode* node) {
  ASSERT(FLAG_enable_type_checks);
  node->expr()->Visit(this);
  __ popl(EAX);
  GenerateAssertAssignable(node->id(),
                           node->token_index(),
                           node->type(),
                           node->dst_name());
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


void CodeGenerator::VisitClosureNode(ClosureNode* node) {
  const Function& function = node->function();
  if (function.IsNonImplicitClosureFunction()) {
    const int current_context_level = state()->context_level();
    const ContextScope& context_scope = ContextScope::ZoneHandle(
        node->scope()->PreserveOuterScope(current_context_level));
    ASSERT(!function.HasCode());
    ASSERT(function.context_scope() == ContextScope::null());
    function.set_context_scope(context_scope);
  } else {
    ASSERT(function.context_scope() != ContextScope::null());
    if (function.IsImplicitInstanceClosureFunction()) {
      node->receiver()->Visit(this);
    }
  }
  // The function type of a closure may have type arguments. In that case, pass
  // the type arguments of the instantiator.
  const Class& cls = Class::Handle(function.signature_class());
  ASSERT(!cls.IsNull());
  const bool requires_type_arguments = cls.HasTypeArguments();
  if (requires_type_arguments) {
    ASSERT(!function.IsImplicitStaticClosureFunction());
    GenerateInstantiatorTypeArguments(node->token_index());
  }
  const Code& stub = Code::Handle(
      StubCode::GetAllocationStubForClosure(function));
  const ExternalLabel label(function.ToCString(), stub.EntryPoint());
  GenerateCall(node->token_index(), &label);
  if (requires_type_arguments) {
    __ popl(ECX);  // Pop type arguments.
  }
  if (function.IsImplicitInstanceClosureFunction()) {
    __ popl(ECX);  // Pop receiver.
  }
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


void CodeGenerator::VisitPrimaryNode(PrimaryNode* node) {
  // PrimaryNodes are temporary during parsing.
  ErrorMsg(node->token_index(),
      "Unexpected primary node: %s", node->primary().ToCString());
}


void CodeGenerator::VisitCloneContextNode(CloneContextNode *node) {
  const Context& result = Context::ZoneHandle();
  __ PushObject(result);
  __ pushl(CTX);
  GenerateCallRuntime(node->id(),
      node->token_index(), kCloneContextRuntimeEntry);
  __ popl(EAX);
  __ popl(CTX);  // result: cloned context. Set as current context.
}


void CodeGenerator::VisitSequenceNode(SequenceNode* node_sequence) {
  CodeGeneratorState codegen_state(this);
  LocalScope* scope = node_sequence->scope();
  ASSERT(scope != NULL);
  intptr_t num_context_variables = scope->num_context_variables();
  if (num_context_variables > 0) {
    // The loop local scope declares variables that are captured.
    // Allocate and chain a new context.
    __ movl(EDX, Immediate(num_context_variables));
    const ExternalLabel label("alloc_context",
                              StubCode::AllocateContextEntryPoint());
    GenerateCall(node_sequence->token_index(), &label);

    // Chain the new context in EAX to its parent in CTX.
    __ movl(FieldAddress(EAX, Context::parent_offset()), CTX);
    // Set new context as current context.
    __ movl(CTX, EAX);
    state()->set_context_level(scope->context_level());

    // If this node_sequence is the body of the function being compiled, copy
    // the captured parameters from the frame into the context.
    if (node_sequence == parsed_function_.node_sequence()) {
      ASSERT(scope->context_level() == 1);
      const Immediate raw_null =
          Immediate(reinterpret_cast<intptr_t>(Object::null()));
      const Function& function = parsed_function_.function();
      const int num_params = function.NumberOfParameters();
      int param_frame_index =
          (num_params == function.num_fixed_parameters()) ? 1 + num_params : -1;
      for (int pos = 0; pos < num_params; param_frame_index--, pos++) {
        LocalVariable* parameter = scope->VariableAt(pos);
        ASSERT(parameter->owner() == scope);
        if (parameter->is_captured()) {
          // Copy parameter from local frame to current context.
          const Address local_addr(EBP, param_frame_index * kWordSize);
          __ movl(EAX, local_addr);
          GenerateStoreVariable(*parameter, EAX, EDX);
          // Write NULL to the source location to detect buggy accesses and
          // allow GC of passed value if it gets overwritten by a new value in
          // the function.
          __ movl(local_addr, raw_null);
        }
      }
    }
  }
  // If this node_sequence is the body of the function being compiled, generate
  // code checking the type of the actual arguments.
  if (FLAG_enable_type_checks &&
      (node_sequence == parsed_function_.node_sequence())) {
    GenerateArgumentTypeChecks();
  }
  for (int i = 0; i < node_sequence->length(); i++) {
    AstNode* child_node = node_sequence->NodeAt(i);
    state()->set_root_node(child_node);
    child_node->Visit(this);
  }
  if (node_sequence->label() != NULL) {
    __ Bind(node_sequence->label()->break_label());
  }
  if (num_context_variables > 0) {
    // Unchain the previously allocated context.
    __ movl(CTX, FieldAddress(CTX, Context::parent_offset()));
  }
}


void CodeGenerator::VisitArgumentListNode(ArgumentListNode* arguments) {
  for (int i = 0; i < arguments->length(); i++) {
    AstNode* argument = arguments->NodeAt(i);
    argument->Visit(this);
  }
}


void CodeGenerator::VisitArrayNode(ArrayNode* node) {
  // Evaluate the array elements.
  for (int i = 0; i < node->length(); i++) {
    AstNode* element = node->ElementAt(i);
    element->Visit(this);
  }

  // Allocate the array.
  //   EDX : Array length as Smi.
  //   ECX : element type for the array.
  __ movl(EDX, Immediate(Smi::RawValue(node->length())));
  const AbstractTypeArguments& element_type = node->type_arguments();
  ASSERT(element_type.IsNull() || element_type.IsInstantiated());
  __ LoadObject(ECX, element_type);
  GenerateCall(node->token_index(), &StubCode::AllocateArrayLabel());

  // Pop the element values from the stack into the array.
  __ leal(ECX, FieldAddress(EAX, Array::data_offset()));
  for (int i = node->length() - 1; i >= 0; i--) {
    __ popl(Address(ECX, i * kWordSize));
  }

  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


void CodeGenerator::VisitLoadLocalNode(LoadLocalNode* node) {
  // Load the value of the local variable and push it onto the expression stack.
  if (IsResultNeeded(node)) {
    GeneratePushVariable(node->local(), EAX);
  }
}


void CodeGenerator::VisitStoreLocalNode(StoreLocalNode* node) {
  node->value()->Visit(this);
  __ popl(EAX);
  if (FLAG_enable_type_checks) {
    GenerateAssertAssignable(node->id(),
                             node->value()->token_index(),
                             node->local().type(),
                             node->local().name());
  }
  GenerateStoreVariable(node->local(), EAX, EDX);
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


void CodeGenerator::VisitLoadInstanceFieldNode(LoadInstanceFieldNode* node) {
  node->instance()->Visit(this);
  MarkDeoptPoint(node->id(), node->token_index());
  __ popl(EAX);  // Instance.
  __ movl(EAX, FieldAddress(EAX, node->field().Offset()));
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


void CodeGenerator::VisitStoreInstanceFieldNode(StoreInstanceFieldNode* node) {
  node->instance()->Visit(this);
  node->value()->Visit(this);
  MarkDeoptPoint(node->id(), node->token_index());
  __ popl(EAX);  // Value.
  if (FLAG_enable_type_checks) {
    GenerateAssertAssignable(node->id(),
                             node->value()->token_index(),
                             AbstractType::ZoneHandle(node->field().type()),
                             String::ZoneHandle(node->field().name()));
  }
  __ popl(EDX);  // Instance.
  __ StoreIntoObject(EDX, FieldAddress(EDX, node->field().Offset()), EAX);
  if (IsResultNeeded(node)) {
    // The result is the input value.
    __ pushl(EAX);
  }
}


// Expects array and index on stack and returns result in EAX.
void CodeGenerator::GenerateLoadIndexed(intptr_t node_id,
                                        intptr_t token_index) {
  // Invoke the [] operator on the receiver object with the index as argument.
  const String& operator_name =
      String::ZoneHandle(String::NewSymbol(Token::Str(Token::kINDEX)));
  const int kNumArguments = 2;  // Receiver and index.
  const Array& kNoArgumentNames = Array::Handle();
  GenerateInstanceCall(node_id,
                       token_index,
                       operator_name,
                       kNumArguments,
                       kNoArgumentNames);
}


void CodeGenerator::VisitLoadIndexedNode(LoadIndexedNode* node) {
  node->array()->Visit(this);
  // Now compute the index.
  node->index_expr()->Visit(this);
  MarkDeoptPoint(node->id(), node->token_index());
  GenerateLoadIndexed(node->id(), node->token_index());
  // Result is in EAX.
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


// Expected arguments.
// TOS(0): value.
// TOS(1): index.
// TOS(2): array.
void CodeGenerator::GenerateStoreIndexed(intptr_t node_id,
                                         intptr_t token_index,
                                         bool preserve_value) {
  // It is not necessary to generate a type test of the assigned value here,
  // because the []= operator will check the type of its incoming arguments.
  if (preserve_value) {
    __ popl(EAX);
    __ popl(EDX);
    __ popl(ECX);
    __ pushl(EAX);  // Preserve stored value.
    __ pushl(ECX);  // Restore arguments.
    __ pushl(EDX);
    __ pushl(EAX);
  }
  // Invoke the []= operator on the receiver object with index and
  // value as arguments.
  const String& operator_name =
      String::ZoneHandle(String::NewSymbol(Token::Str(Token::kASSIGN_INDEX)));
  const int kNumArguments = 3;  // Receiver, index and value.
  const Array& kNoArgumentNames = Array::Handle();
  GenerateInstanceCall(node_id,
                       token_index,
                       operator_name,
                       kNumArguments,
                       kNoArgumentNames);
}


void CodeGenerator::VisitStoreIndexedNode(StoreIndexedNode* node) {
  // Compute the receiver object and pass as first argument to call.
  node->array()->Visit(this);
  // Now compute the index.
  node->index_expr()->Visit(this);
  // Finally compute the value to assign.
  node->value()->Visit(this);
  MarkDeoptPoint(node->id(), node->token_index());
  GenerateStoreIndexed(node->id(), node->token_index(), IsResultNeeded(node));
}


void CodeGenerator::VisitLoadStaticFieldNode(LoadStaticFieldNode* node) {
  MarkDeoptPoint(node->id(), node->token_index());
  __ LoadObject(EDX, node->field());
  __ movl(EAX, FieldAddress(EDX, Field::value_offset()));
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


void CodeGenerator::VisitStoreStaticFieldNode(StoreStaticFieldNode* node) {
  node->value()->Visit(this);
  MarkDeoptPoint(node->id(), node->token_index());
  __ popl(EAX);  // Value.
  if (FLAG_enable_type_checks) {
    GenerateAssertAssignable(node->id(),
                             node->value()->token_index(),
                             AbstractType::ZoneHandle(node->field().type()),
                             String::ZoneHandle(node->field().name()));
  }
  __ LoadObject(EDX, node->field());
  __ StoreIntoObject(EDX, FieldAddress(EDX, Field::value_offset()), EAX);
  if (IsResultNeeded(node)) {
    // The result is the input value.
    __ pushl(EAX);
  }
}


void CodeGenerator::GenerateLogicalNotOp(UnaryOpNode* node) {
  // Generate false if operand is true, otherwise generate true.
  const Bool& bool_true = Bool::ZoneHandle(Bool::True());
  const Bool& bool_false = Bool::ZoneHandle(Bool::False());
  node->operand()->Visit(this);
  MarkDeoptPoint(node->id(), node->token_index());
  Label done;
  GenerateConditionTypeCheck(node->id(), node->operand()->token_index());
  __ popl(EDX);
  __ LoadObject(EAX, bool_true);
  __ cmpl(EAX, EDX);
  __ j(NOT_EQUAL, &done, Assembler::kNearJump);
  __ LoadObject(EAX, bool_false);
  __ Bind(&done);
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


void CodeGenerator::VisitUnaryOpNode(UnaryOpNode* node) {
  if (node->kind() == Token::kNOT) {
    // "!" cannot be overloaded, therefore inline it.
    GenerateLogicalNotOp(node);
    return;
  }
  node->operand()->Visit(this);
  if (node->kind() == Token::kADD) {
    // Unary operator '+' does not exist, it's a NOP, skip it.
    if (!IsResultNeeded(node)) {
      __ popl(EAX);
    }
    return;
  }
  MarkDeoptPoint(node->id(), node->token_index());
  String& operator_name = String::ZoneHandle();
  if (node->kind() == Token::kSUB) {
    operator_name = String::NewSymbol(Token::Str(Token::kNEGATE));
  } else {
    operator_name = String::NewSymbol(node->Name());
  }
  const int kNumberOfArguments = 1;
  const Array& kNoArgumentNames = Array::Handle();
  GenerateInstanceCall(node->id(),
                       node->token_index(),
                       operator_name,
                       kNumberOfArguments,
                       kNoArgumentNames);
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


void CodeGenerator::VisitIncrOpLocalNode(IncrOpLocalNode* node) {
  ASSERT((node->kind() == Token::kINCR) || (node->kind() == Token::kDECR));
  MarkDeoptPoint(node->id(), node->token_index());
  GenerateLoadVariable(EAX, node->local());
  if (!node->prefix() && IsResultNeeded(node)) {
    // Preserve as result.
    __ pushl(EAX);
  }
  const Immediate value = Immediate(reinterpret_cast<int32_t>(Smi::New(1)));
  const char* operator_name = (node->kind() == Token::kINCR) ? "+" : "-";
  __ pushl(EAX);
  __ pushl(value);
  GenerateBinaryOperatorCall(node->id(), node->token_index(), operator_name);
  // result is in EAX.
  if (FLAG_enable_type_checks) {
    GenerateAssertAssignable(node->id(),
                             node->token_index(),
                             node->local().type(),
                             node->local().name());
  }
  GenerateStoreVariable(node->local(), EAX, EDX);
  if (node->prefix() && IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


void CodeGenerator::VisitIncrOpInstanceFieldNode(
    IncrOpInstanceFieldNode* node) {
  ASSERT((node->kind() == Token::kINCR) || (node->kind() == Token::kDECR));
  node->receiver()->Visit(this);
  __ pushl(Address(ESP, 0));  // Duplicate receiver (preserve for setter).
  MarkDeoptPoint(node->getter_id(), node->token_index());
  GenerateInstanceGetterCall(node->getter_id(),
                             node->token_index(),
                             node->field_name());
  // result is in EAX.
  __ popl(EDX);   // Get receiver.
  if (!node->prefix() && IsResultNeeded(node)) {
    // Preserve as result.
    __ pushl(EAX);  // Preserve value as result.
  }
  const Immediate one_value = Immediate(reinterpret_cast<int32_t>(Smi::New(1)));
  const char* operator_name = (node->kind() == Token::kINCR) ? "+" : "-";
  // EAX: Value.
  // EDX: Receiver.
  __ pushl(EDX);  // Preserve receiver.
  __ pushl(EAX);  // Left operand.
  __ pushl(one_value);  // Right operand.
  GenerateBinaryOperatorCall(node->operator_id(),
                             node->token_index(),
                             operator_name);
  __ popl(EDX);  // Restore receiver.
  if (IsResultNeeded(node) && node->prefix()) {
    // Value stored into field is the result.
    __ pushl(EAX);
  }
  __ pushl(EDX);  // Receiver.
  __ pushl(EAX);  // Value.
  // It is not necessary to generate a type test of the assigned value here,
  // because the setter will check the type of its incoming arguments.
  GenerateInstanceSetterCall(node->setter_id(),
                             node->token_index(),
                             node->field_name());
}


void CodeGenerator::VisitIncrOpStaticFieldNode(IncrOpStaticFieldNode* node) {
  ASSERT((node->kind() == Token::kINCR) || (node->kind() == Token::kDECR));
  MarkDeoptPoint(node->id(), node->token_index());
  if (node->field().IsNull()) {
    GenerateStaticGetterCall(node->token_index(),
                             node->field_class(),
                             node->field_name());
  } else {
    __ LoadObject(EDX, node->field());
    __ movl(EAX, FieldAddress(EDX, Field::value_offset()));
  }
  // Value in EAX.
  if (!node->prefix() && IsResultNeeded(node)) {
    // Preserve as result.
    __ pushl(EAX);
  }
  const Immediate value = Immediate(reinterpret_cast<int32_t>(Smi::New(1)));
  const char* operator_name = (node->kind() == Token::kINCR) ? "+" : "-";
  __ pushl(EAX);    // Left operand.
  __ pushl(value);  // Right operand.
  GenerateBinaryOperatorCall(node->id(), node->token_index(), operator_name);
  // result is in EAX.
  if (node->prefix() && IsResultNeeded(node)) {
    __ pushl(EAX);
  }
  if (node->field().IsNull()) {
    __ pushl(EAX);
    // It is not necessary to generate a type test of the assigned value here,
    // because the setter will check the type of its incoming arguments.
    GenerateStaticSetterCall(node->token_index(),
                             node->field_class(),
                             node->field_name());
  } else {
    if (FLAG_enable_type_checks) {
      GenerateAssertAssignable(node->id(),
                               node->token_index(),
                               AbstractType::ZoneHandle(node->field().type()),
                               String::ZoneHandle(node->field().name()));
    }
    __ LoadObject(EDX, node->field());
    __ StoreIntoObject(EDX, FieldAddress(EDX, Field::value_offset()), EAX);
  }
}


void CodeGenerator::VisitIncrOpIndexedNode(IncrOpIndexedNode* node) {
  ASSERT((node->kind() == Token::kINCR) || (node->kind() == Token::kDECR));
  node->array()->Visit(this);
  node->index()->Visit(this);
  MarkDeoptPoint(node->id(), node->token_index());
  // Preserve array and index for GenerateStoreIndex.
  __ pushl(Address(ESP, kWordSize));  // Copy array.
  __ pushl(Address(ESP, kWordSize));  // Copy index.
  GenerateLoadIndexed(node->load_id(), node->token_index());
  // Result is in EAX.
  if (!node->prefix() && IsResultNeeded(node)) {
    // Preserve EAX as result.
    __ popl(EDX);  // Preserved index -> EDX.
    __ popl(ECX);  // Preserved array -> ECX.
    __ pushl(EAX);  // Preserve original value from indexed load.
    __ pushl(ECX);  // Array.
    __ pushl(EDX);  // Index.
  }
  const Immediate value = Immediate(reinterpret_cast<int32_t>(Smi::New(1)));
  const char* operator_name = (node->kind() == Token::kINCR) ? "+" : "-";
  __ pushl(EAX);    // Left operand.
  __ pushl(value);  // Right operand.
  GenerateBinaryOperatorCall(node->operator_id(),
                             node->token_index(),
                             operator_name);
  __ pushl(EAX);
  // TOS(0): value, TOS(1): index, TOS(2): array.
  GenerateStoreIndexed(node->store_id(),
                       node->token_index(),
                       node->prefix() && IsResultNeeded(node));
}


static const Class* CoreClass(const char* c_name) {
  const String& class_name = String::Handle(String::NewSymbol(c_name));
  const Class& cls = Class::ZoneHandle(Library::Handle(
      Library::CoreImplLibrary()).LookupClass(class_name));
  ASSERT(!cls.IsNull());
  return &cls;
}


// Optimize instanceof type test by adding inlined tests for:
// - NULL -> return false.
// - Smi -> compile time subtype check (only if dst class is not parameterized).
// - Class equality (only if class is not parameterized).
// Inputs:
// - EAX: object.
// Destroys ECX.
// Returns:
// - true or false on stack.
void CodeGenerator::GenerateInstanceOf(intptr_t node_id,
                                       intptr_t token_index,
                                       const AbstractType& type,
                                       bool negate_result) {
  ASSERT(type.IsFinalized());
  const Bool& bool_true = Bool::ZoneHandle(Bool::True());
  const Bool& bool_false = Bool::ZoneHandle(Bool::False());

  // All instances are of a subtype of the Object type.
  const Type& object_type =
      Type::Handle(Isolate::Current()->object_store()->object_type());
  if (type.IsInstantiated() && object_type.IsSubtypeOf(type)) {
    __ PushObject(negate_result ? bool_false : bool_true);
    return;
  }

  const Immediate raw_null =
      Immediate(reinterpret_cast<intptr_t>(Object::null()));
  Label done;
  // If type is instantiated and non-parameterized, we can inline code
  // checking whether the tested instance is a Smi.
  if (type.IsInstantiated()) {
    // A null object is only an instance of Object and Dynamic, which has
    // already been checked above (if the type is instantiated). So we can
    // return false here if the instance is null (and if the type is
    // instantiated).
    // We can only inline this null check if the type is instantiated at compile
    // time, since an uninstantiated type at compile time could be Object or
    // Dynamic at run time.
    Label non_null;
    __ cmpl(EAX, raw_null);
    __ j(NOT_EQUAL, &non_null, Assembler::kNearJump);
    __ PushObject(negate_result ? bool_true : bool_false);
    __ jmp(&done, Assembler::kNearJump);

    __ Bind(&non_null);

    const Class& type_class = Class::ZoneHandle(type.type_class());
    const bool requires_type_arguments = type_class.HasTypeArguments();
    // A Smi object cannot be the instance of a parameterized class.
    // A class equality check is only applicable with a dst type of a
    // non-parameterized class or with a raw dst type of a parameterized class.
    if (requires_type_arguments) {
      const AbstractTypeArguments& type_arguments =
          AbstractTypeArguments::Handle(type.arguments());
      const bool is_raw_type = type_arguments.IsNull() ||
          type_arguments.IsDynamicTypes(type_arguments.Length());
      Label runtime_call;
      __ testl(EAX, Immediate(kSmiTagMask));
      __ j(ZERO, &runtime_call, Assembler::kNearJump);
      // Object not Smi.
      if (is_raw_type) {
        if (type.IsListInterface()) {
          Label push_result;
          // TODO(srdjan) also accept List<Object>.
          __ movl(ECX, FieldAddress(EAX, Object::class_offset()));
          __ CompareObject(ECX, *CoreClass("ObjectArray"));
          __ j(EQUAL, &push_result, Assembler::kNearJump);
          __ CompareObject(ECX, *CoreClass("GrowableObjectArray"));
          __ j(NOT_EQUAL, &runtime_call, Assembler::kNearJump);
          __ Bind(&push_result);
          __ PushObject(negate_result ? bool_false : bool_true);
          __ jmp(&done, Assembler::kNearJump);
        } else if (!type_class.is_interface()) {
          __ movl(ECX, FieldAddress(EAX, Object::class_offset()));
          __ CompareObject(ECX, type_class);
          __ j(NOT_EQUAL, &runtime_call, Assembler::kNearJump);
          __ PushObject(negate_result ? bool_false : bool_true);
          __ jmp(&done, Assembler::kNearJump);
        }
      }
      __ Bind(&runtime_call);
      // Fall through to runtime call.
    } else {
      Label compare_classes;
      __ testl(EAX, Immediate(kSmiTagMask));
      __ j(NOT_ZERO, &compare_classes, Assembler::kNearJump);
      // Object is Smi.
      const Class& smi_class = Class::Handle(Smi::Class());
      // TODO(regis): We should introduce a SmiType.
      if (smi_class.IsSubtypeOf(TypeArguments::Handle(),
                                type_class,
                                TypeArguments::Handle())) {
        __ PushObject(negate_result ? bool_false : bool_true);
      } else {
        __ PushObject(negate_result ? bool_true : bool_false);
      }
      __ jmp(&done, Assembler::kNearJump);

      // Compare if the classes are equal.
      __ Bind(&compare_classes);
      const Class* compare_class = NULL;
      if (type.IsStringInterface()) {
        compare_class = &Class::ZoneHandle(
            Isolate::Current()->object_store()->one_byte_string_class());
      } else if (type.IsBoolInterface()) {
        compare_class = &Class::ZoneHandle(
            Isolate::Current()->object_store()->bool_class());
      } else if (!type_class.is_interface()) {
        compare_class = &type_class;
      }
      if (compare_class != NULL) {
        Label runtime_call;
        __ movl(ECX, FieldAddress(EAX, Object::class_offset()));
        __ CompareObject(ECX, *compare_class);
        __ j(NOT_EQUAL, &runtime_call, Assembler::kNearJump);
        __ PushObject(negate_result ? bool_false : bool_true);
        __ jmp(&done, Assembler::kNearJump);
        __ Bind(&runtime_call);
      }
    }
  }
  const Object& result = Object::ZoneHandle();
  __ PushObject(result);  // Make room for the result of the runtime call.
  __ pushl(EAX);  // Push the instance.
  __ PushObject(type);  // Push the type.
  if (!type.IsInstantiated()) {
    GenerateInstantiatorTypeArguments(token_index);
  } else {
    __ pushl(raw_null);  // Null instantiator.
  }
  GenerateCallRuntime(node_id, token_index, kInstanceofRuntimeEntry);
  // Pop the two parameters supplied to the runtime entry. The result of the
  // instanceof runtime call will be left as the result of the operation.
  __ addl(ESP, Immediate(3 * kWordSize));
  if (negate_result) {
    Label negate_done;
    __ popl(EDX);
    __ LoadObject(EAX, bool_true);
    __ cmpl(EDX, EAX);
    __ j(NOT_EQUAL, &negate_done, Assembler::kNearJump);
    __ LoadObject(EAX, bool_false);
    __ Bind(&negate_done);
    __ pushl(EAX);
  }
  __ Bind(&done);
}


// Jumps to label if ECX equals the given class.
// Inputs:
// - ECX: tested class.
void CodeGenerator::TestClassAndJump(const Class& cls, Label* label) {
  __ CompareObject(ECX, cls);
  __ j(EQUAL, label, Assembler::kNearJump);
}


// Optimize assignable type check by adding inlined tests for:
// - NULL -> return NULL.
// - Smi -> compile time subtype check (only if dst class is not parameterized).
// - Class equality (only if class is not parameterized).
// Inputs:
// - EAX: object.
// Destroys ECX and EDX.
// Returns:
// - object in EAX for successful assignable check (or throws TypeError).
void CodeGenerator::GenerateAssertAssignable(intptr_t node_id,
                                             intptr_t token_index,
                                             const AbstractType& dst_type,
                                             const String& dst_name) {
  ASSERT(FLAG_enable_type_checks);
  ASSERT(token_index >= 0);
  ASSERT(!dst_type.IsNull());
  ASSERT(dst_type.IsFinalized());

  // Any expression is assignable to the Dynamic type and to the Object type.
  // Skip the test.
  if (dst_type.IsDynamicType() || dst_type.IsObjectType()) {
    return;
  }

  // It is a compile-time error to explicitly return a value (including null)
  // from a void function. However, functions that do not explicitly return a
  // value, implicitly return null. This includes void functions. Therefore, we
  // skip the type test here and trust the parser to only return null in void
  // function.
  if (dst_type.IsVoidType()) {
    return;
  }

  // A NULL object is always assignable and is returned as result.
  const Immediate raw_null =
      Immediate(reinterpret_cast<intptr_t>(Object::null()));
  Label done, runtime_call;
  __ cmpl(EAX, raw_null);
  __ j(EQUAL, &done, Assembler::kNearJump);

  // If dst_type is instantiated and non-parameterized, we can inline code
  // checking whether the assigned instance is a Smi.
  if (dst_type.IsInstantiated()) {
    const Class& dst_type_class = Class::ZoneHandle(dst_type.type_class());
    const bool dst_class_has_type_arguments = dst_type_class.HasTypeArguments();
    // A Smi object cannot be the instance of a parameterized class.
    // A class equality check is only applicable with a dst type of a
    // non-parameterized class or with a raw dst type of a parameterized class.
    if (dst_class_has_type_arguments) {
      const AbstractTypeArguments& dst_type_arguments =
          AbstractTypeArguments::Handle(dst_type.arguments());
      const bool is_raw_dst_type = dst_type_arguments.IsNull() ||
          dst_type_arguments.IsDynamicTypes(dst_type_arguments.Length());
      if (is_raw_dst_type) {
        // Dynamic type argument, check only classes.
        if (dst_type.IsListInterface()) {
          // TODO(srdjan) also accept List<Object>.
          __ testl(EAX, Immediate(kSmiTagMask));
          __ j(ZERO, &runtime_call, Assembler::kNearJump);
          __ movl(ECX, FieldAddress(EAX, Object::class_offset()));
          TestClassAndJump(*CoreClass("ObjectArray"), &done);
          TestClassAndJump(*CoreClass("GrowableObjectArray"), &done);
        } else if (!dst_type_class.is_interface()) {
          __ testl(EAX, Immediate(kSmiTagMask));
          __ j(ZERO, &runtime_call, Assembler::kNearJump);
          __ movl(ECX, FieldAddress(EAX, Object::class_offset()));
          TestClassAndJump(dst_type_class, &done);
        }
        // Fall through to runtime class.
      }
    } else {  // dst_type has NO type arguments.
      Label compare_classes;
      __ testl(EAX, Immediate(kSmiTagMask));
      __ j(NOT_ZERO, &compare_classes, Assembler::kNearJump);
      // Object is Smi.
      const Class& smi_class = Class::Handle(Smi::Class());
      // TODO(regis): We should introduce a SmiType.
      if (smi_class.IsSubtypeOf(TypeArguments::Handle(),
                                dst_type_class,
                                TypeArguments::Handle())) {
        // Successful assignable type check: return object in EAX.
        __ jmp(&done, Assembler::kNearJump);
      } else {
        // Failed assignable type check: call runtime to throw TypeError.
        __ jmp(&runtime_call, Assembler::kNearJump);
      }
      // Compare if the classes are equal.
      __ Bind(&compare_classes);
      // If dst_type is an interface, we can skip the class equality check,
      // because instances cannot be of an interface type.
      if (!dst_type_class.is_interface()) {
        __ movl(ECX, FieldAddress(EAX, Object::class_offset()));
        TestClassAndJump(dst_type_class, &done);
      } else {
        // However, for specific core library interfaces, we can check for
        // specific core library classes.
        if (dst_type.IsBoolInterface()) {
          __ movl(ECX, FieldAddress(EAX, Object::class_offset()));
          const Class& bool_class = Class::ZoneHandle(
              Isolate::Current()->object_store()->bool_class());
          TestClassAndJump(bool_class, &done);
        } else if (dst_type.IsSubtypeOf(
              Type::Handle(Type::NumberInterface()))) {
          __ movl(ECX, FieldAddress(EAX, Object::class_offset()));
          if (dst_type.IsIntInterface() || dst_type.IsNumberInterface()) {
            // We already checked for Smi above.
            const Class& mint_class = Class::ZoneHandle(
                Isolate::Current()->object_store()->mint_class());
            TestClassAndJump(mint_class, &done);
            const Class& bigint_class = Class::ZoneHandle(
                Isolate::Current()->object_store()->bigint_class());
            TestClassAndJump(bigint_class, &done);
          }
          if (dst_type.IsDoubleInterface() || dst_type.IsNumberInterface()) {
            const Class& double_class = Class::ZoneHandle(
                Isolate::Current()->object_store()->double_class());
            TestClassAndJump(double_class, &done);
          }
        } else if (dst_type.IsStringInterface()) {
          __ movl(ECX, FieldAddress(EAX, Object::class_offset()));
          const Class& one_byte_string_class = Class::ZoneHandle(
              Isolate::Current()->object_store()->one_byte_string_class());
          TestClassAndJump(one_byte_string_class, &done);
          const Class& two_byte_string_class = Class::ZoneHandle(
              Isolate::Current()->object_store()->two_byte_string_class());
          TestClassAndJump(two_byte_string_class, &done);
          const Class& four_byte_string_class = Class::ZoneHandle(
              Isolate::Current()->object_store()->four_byte_string_class());
          TestClassAndJump(four_byte_string_class, &done);
        } else if (dst_type.IsFunctionInterface()) {
          __ movl(ECX, FieldAddress(EAX, Object::class_offset()));
          __ movl(ECX, FieldAddress(ECX, Class::signature_function_offset()));
          __ cmpl(ECX, raw_null);
          __ j(NOT_EQUAL, &done, Assembler::kNearJump);
        }
      }
    }
  }
  __ Bind(&runtime_call);
  const Object& result = Object::ZoneHandle();
  __ PushObject(result);  // Make room for the result of the runtime call.
  const Immediate location =
      Immediate(reinterpret_cast<int32_t>(Smi::New(token_index)));
  __ pushl(location);  // Push the source location.
  __ pushl(EAX);  // Push the source object.
  __ PushObject(dst_type);  // Push the type of the destination.
  if (!dst_type.IsInstantiated()) {
    GenerateInstantiatorTypeArguments(token_index);
  } else {
    __ pushl(raw_null);  // Null instantiator.
  }
  __ PushObject(dst_name);  // Push the name of the destination.
  GenerateCallRuntime(node_id, token_index, kTypeCheckRuntimeEntry);
  // Pop the parameters supplied to the runtime entry. The result of the
  // type check runtime call is the checked value.
  __ addl(ESP, Immediate(5 * kWordSize));
  __ popl(EAX);

  __ Bind(&done);
}


void CodeGenerator::GenerateArgumentTypeChecks() {
  const Function& function = parsed_function_.function();
  LocalScope* scope = parsed_function_.node_sequence()->scope();
  const int num_fixed_params = function.num_fixed_parameters();
  const int num_opt_params = function.num_optional_parameters();
  ASSERT(num_fixed_params + num_opt_params <= scope->num_variables());
  for (int i = 0; i < num_fixed_params + num_opt_params; i++) {
    LocalVariable* parameter = scope->VariableAt(i);
    GenerateLoadVariable(EAX, *parameter);
    GenerateAssertAssignable(AstNode::kNoId,
                             parameter->token_index(),
                             parameter->type(),
                             parameter->name());
  }
}


void CodeGenerator::GenerateConditionTypeCheck(intptr_t node_id,
                                               intptr_t token_index) {
  if (!FLAG_enable_type_checks) {
    return;
  }

  // Check that the type of the object on the stack is allowed in conditional
  // context.
  // Call the runtime if the object is null or not of type bool.
  const Immediate raw_null =
      Immediate(reinterpret_cast<intptr_t>(Object::null()));
  Label runtime_call, done;
  __ movl(EAX, Address(ESP, 0));
  __ cmpl(EAX, raw_null);
  __ j(EQUAL, &runtime_call, Assembler::kNearJump);
  __ testl(EAX, Immediate(kSmiTagMask));
  __ j(ZERO, &runtime_call, Assembler::kNearJump);  // Call runtime for Smi.
  // This check should pass if the receiver's class implements the interface
  // 'bool'. Check only class 'Bool' since it is the only legal implementation
  // of the interface 'bool'.
  const Class& bool_class =
      Class::ZoneHandle(Isolate::Current()->object_store()->bool_class());
  __ movl(ECX, FieldAddress(EAX, Object::class_offset()));
  __ CompareObject(ECX, bool_class);
  __ j(EQUAL, &done, Assembler::kNearJump);

  __ Bind(&runtime_call);
  const Object& result = Object::ZoneHandle();
  __ PushObject(result);  // Make room for the result of the runtime call.
  const Immediate location =
      Immediate(reinterpret_cast<int32_t>(Smi::New(token_index)));
  __ pushl(location);  // Push the source location.
  __ pushl(EAX);  // Push the source object.
  GenerateCallRuntime(node_id, token_index, kConditionTypeErrorRuntimeEntry);
  // Pop the parameters supplied to the runtime entry. The result of the
  // type check runtime call is the checked value.
  __ addl(ESP, Immediate(3 * kWordSize));

  __ Bind(&done);
}


void CodeGenerator::VisitComparisonNode(ComparisonNode* node) {
  const Bool& bool_true = Bool::ZoneHandle(Bool::True());
  const Bool& bool_false = Bool::ZoneHandle(Bool::False());
  node->left()->Visit(this);

  // The instanceof operator needs special handling.
  if (Token::IsInstanceofOperator(node->kind())) {
    __ popl(EAX);  // Left operand.
    ASSERT(node->right()->IsTypeNode());
    GenerateInstanceOf(node->id(),
                       node->token_index(),
                       node->right()->AsTypeNode()->type(),
                       (node->kind() == Token::kISNOT));
    if (!IsResultNeeded(node)) {
      __ popl(EAX);  // Pop the result of the instanceof operation.
    }
    return;
  }

  node->right()->Visit(this);
  // Both left and right values on stack.

  // '===' and '!==' are not overloadable.
  if ((node->kind() == Token::kEQ_STRICT) ||
      (node->kind() == Token::kNE_STRICT)) {
    __ popl(EDX);  // Right operand.
    __ popl(EAX);  // Left operand.
    if (!IsResultNeeded(node)) {
      return;
    }
    Label load_true, done;
    __ cmpl(EAX, EDX);
    if (node->kind() == Token::kEQ_STRICT) {
      __ j(EQUAL, &load_true, Assembler::kNearJump);
    } else {
      __ j(NOT_EQUAL, &load_true, Assembler::kNearJump);
    }
    __ LoadObject(EAX, bool_false);
    __ jmp(&done, Assembler::kNearJump);
    __ Bind(&load_true);
    __ LoadObject(EAX, bool_true);
    __ Bind(&done);
    // Result is in EAX.
    __ pushl(EAX);
    return;
  }

  MarkDeoptPoint(node->id(), node->token_index());

  // '!=' not overloadable, always implements negation of '=='.
  // Call operator for '=='.
  if ((node->kind() == Token::kEQ) || (node->kind() == Token::kNE)) {
    // Null is a special receiver with a special type and frequently used on
    // operators "==" and "!=". Emit inlined code for null so that it does not
    // pollute type information at call site.
    Label null_done;
    {
      const Immediate raw_null =
          Immediate(reinterpret_cast<intptr_t>(Object::null()));
      Label non_null_compare, load_true;
      // Check if left argument is null.
      __ cmpl(Address(ESP, 1 * kWordSize), raw_null);
      __ j(NOT_EQUAL, &non_null_compare, Assembler::kNearJump);
      // Comparison with NULL is "===".
      // Load/remove arguments.
      __ popl(EDX);
      __ popl(EAX);
      __ cmpl(EAX, EDX);
      if (node->kind() == Token::kEQ) {
        __ j(EQUAL, &load_true, Assembler::kNearJump);
      } else {
        __ j(NOT_EQUAL, &load_true, Assembler::kNearJump);
      }
      __ LoadObject(EAX, bool_false);
      __ jmp(&null_done, Assembler::kNearJump);
      __ Bind(&load_true);
      __ LoadObject(EAX, bool_true);
      __ jmp(&null_done, Assembler::kNearJump);
      __ Bind(&non_null_compare);
    }
    // Do '==' first then negate if necessary,
    const String& operator_name = String::ZoneHandle(String::NewSymbol("=="));
    const int kNumberOfArguments = 2;
    const Array& kNoArgumentNames = Array::Handle();
    GenerateInstanceCall(node->id(),
                         node->token_index(),
                         operator_name,
                         kNumberOfArguments,
                         kNoArgumentNames);

    // Result is in EAX. No need to negate if result is not needed.
    if ((node->kind() == Token::kNE) && IsResultNeeded(node)) {
      // Negate result.
      Label load_true, done;
      __ LoadObject(EDX, bool_false);
      __ cmpl(EAX, EDX);
      __ j(EQUAL, &load_true, Assembler::kNearJump);
      __ movl(EAX, EDX);  // false.
      __ jmp(&done, Assembler::kNearJump);
      __ Bind(&load_true);
      __ LoadObject(EAX, bool_true);
      __ Bind(&done);
    }
    __ Bind(&null_done);
    // Result is in EAX.
    if (IsResultNeeded(node)) {
      __ pushl(EAX);
    }
    return;
  }

  // Call operator.
  GenerateBinaryOperatorCall(node->id(), node->token_index(), node->Name());
  // Result is in EAX.
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


void CodeGenerator::CountBackwardLoop() {
  Label done;
  const Function& function =
      Function::ZoneHandle(parsed_function_.function().raw());
  __ LoadObject(EAX, function);
  __ movl(EBX, FieldAddress(EAX, Function::invocation_counter_offset()));
  __ incl(EBX);
  if (!FLAG_report_invocation_count) {
    // Prevent overflow.
    __ cmpl(EBX, Immediate(FLAG_optimization_invocation_threshold));
    __ j(GREATER, &done);
  }
  __ movl(FieldAddress(EAX, Function::invocation_counter_offset()), EBX);
  __ Bind(&done);
}


void CodeGenerator::VisitWhileNode(WhileNode* node) {
  const Bool& bool_true = Bool::ZoneHandle(Bool::True());
  SourceLabel* label = node->label();
  __ Bind(label->continue_label());
  node->condition()->Visit(this);
  GenerateConditionTypeCheck(node->id(), node->condition()->token_index());
  __ popl(EAX);
  __ LoadObject(EDX, bool_true);
  __ cmpl(EAX, EDX);
  __ j(NOT_EQUAL, label->break_label());
  node->body()->Visit(this);
  CountBackwardLoop();
  __ jmp(label->continue_label());
  __ Bind(label->break_label());
}


void CodeGenerator::VisitDoWhileNode(DoWhileNode* node) {
  const Bool& bool_true = Bool::ZoneHandle(Bool::True());
  SourceLabel* label = node->label();
  Label loop;
  __ Bind(&loop);
  node->body()->Visit(this);
  CountBackwardLoop();
  __ Bind(label->continue_label());
  node->condition()->Visit(this);
  GenerateConditionTypeCheck(node->id(), node->condition()->token_index());
  __ popl(EAX);
  __ LoadObject(EDX, bool_true);
  __ cmpl(EAX, EDX);
  __ j(EQUAL, &loop);
  __ Bind(label->break_label());
}


void CodeGenerator::VisitForNode(ForNode* node) {
  const Bool& bool_true = Bool::ZoneHandle(Bool::True());
  node->initializer()->Visit(this);
  SourceLabel* label = node->label();
  Label loop;
  __ Bind(&loop);
  if (node->condition() != NULL) {
    node->condition()->Visit(this);
    GenerateConditionTypeCheck(node->id(), node->condition()->token_index());
    __ popl(EAX);
    __ LoadObject(EDX, bool_true);
    __ cmpl(EAX, EDX);
    __ j(NOT_EQUAL, label->break_label());
  }
  node->body()->Visit(this);
  CountBackwardLoop();
  __ Bind(label->continue_label());
  node->increment()->Visit(this);
  __ jmp(&loop);
  __ Bind(label->break_label());
}


void CodeGenerator::VisitJumpNode(JumpNode* node) {
  SourceLabel* label = node->label();

  // Generate inlined code for all finally blocks as we may transfer
  // control out of the 'try' blocks if any.
  for (intptr_t i = 0; i < node->inlined_finally_list_length(); i++) {
    node->InlinedFinallyNodeAt(i)->Visit(this);
  }

  // Unchain the context(s) up to the outer context level of the scope which
  // contains the destination label.
  ASSERT(label->owner() != NULL);
  LocalScope* outer_context_owner = label->owner()->parent();
  ASSERT(outer_context_owner != NULL);
  int target_context_level = 0;
  if (outer_context_owner->HasContextLevel()) {
    target_context_level = outer_context_owner->context_level();
    ASSERT(target_context_level >= 0);
    int context_level = state()->context_level();
    ASSERT(context_level >= target_context_level);
    while (context_level-- > target_context_level) {
      __ movl(CTX, FieldAddress(CTX, Context::parent_offset()));
    }
  }

  if (node->kind() == Token::kBREAK) {
    __ jmp(label->break_label());
  } else {
    __ jmp(label->continue_label());
  }
}


void CodeGenerator::VisitConditionalExprNode(ConditionalExprNode* node) {
  const Bool& bool_true = Bool::ZoneHandle(Bool::True());
  Label false_label, done;
  node->condition()->Visit(this);
  GenerateConditionTypeCheck(node->id(), node->condition()->token_index());
  __ popl(EAX);
  __ LoadObject(EDX, bool_true);
  __ cmpl(EAX, EDX);
  __ j(NOT_EQUAL, &false_label);
  node->true_expr()->Visit(this);
  __ jmp(&done);
  __ Bind(&false_label);
  node->false_expr()->Visit(this);
  __ Bind(&done);
  if (!IsResultNeeded(node)) {
    __ popl(EAX);
  }
}


void CodeGenerator::VisitSwitchNode(SwitchNode *node) {
  SourceLabel* label = node->label();
  node->body()->Visit(this);
  __ Bind(label->break_label());
}


void CodeGenerator::VisitCaseNode(CaseNode* node) {
  const Bool& bool_true = Bool::ZoneHandle(Bool::True());
  Label case_statements, end_case;

  for (int i = 0; i < node->case_expressions()->length(); i++) {
    // Load case expression onto stack.
    AstNode* case_expr = node->case_expressions()->NodeAt(i);
    case_expr->Visit(this);
    __ popl(EAX);
    __ CompareObject(EAX, bool_true);
    // Jump to case clause code if case expression equals switch expression
    __ j(EQUAL, &case_statements);
  }
  // If this case clause contains the default label, fall through to
  // case clause code, else skip this clause.
  if (!node->contains_default()) {
    __ jmp(&end_case);
  }

  // If there is a label associated with this case clause, bind it.
  if (node->label() != NULL) {
    __ Bind(node->label()->continue_label());
  }

  // Generate code for case clause statements. The parser guarantees that
  // the code contains a jump, so we should never fall through the end
  // of the statements.
  __ Bind(&case_statements);
  node->statements()->Visit(this);
  __ Bind(&end_case);
}


void CodeGenerator::VisitIfNode(IfNode* node) {
  const Bool& bool_true = Bool::ZoneHandle(Bool::True());
  Label false_label;
  node->condition()->Visit(this);
  GenerateConditionTypeCheck(node->id(), node->condition()->token_index());
  __ popl(EAX);
  __ LoadObject(EDX, bool_true);
  __ cmpl(EAX, EDX);
  __ j(NOT_EQUAL, &false_label);
  node->true_branch()->Visit(this);
  if (node->false_branch() != NULL) {
    Label done;
    __ jmp(&done);
    __ Bind(&false_label);
    node->false_branch()->Visit(this);
    __ Bind(&done);
  } else {
    __ Bind(&false_label);
  }
}


// Operators '&&' and '||' are not overloadabled, inline them.
void CodeGenerator::GenerateLogicalAndOrOp(BinaryOpNode* node) {
  // Generate true if (left == true) op (right == true), otherwise generate
  // false, with op being either || or &&.
  const Bool& bool_true = Bool::ZoneHandle(Bool::True());
  const Bool& bool_false = Bool::ZoneHandle(Bool::False());
  Label load_false, done;
  node->left()->Visit(this);
  GenerateConditionTypeCheck(node->id(), node->left()->token_index());
  __ popl(EAX);
  __ LoadObject(EDX, bool_true);
  __ cmpl(EAX, EDX);
  if (node->kind() == Token::kAND) {
    __ j(NOT_EQUAL, &load_false);
  } else {
    ASSERT(node->kind() == Token::kOR);
    __ j(EQUAL, &done);
  }
  node->right()->Visit(this);
  GenerateConditionTypeCheck(node->id(), node->right()->token_index());
  __ popl(EAX);
  __ LoadObject(EDX, bool_true);
  __ cmpl(EAX, EDX);
  __ j(EQUAL, &done);
  __ Bind(&load_false);
  __ LoadObject(EAX, bool_false);
  __ Bind(&done);
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


// Expect receiver(left operand) and right operand on stack.
// Return result in EAX.
void CodeGenerator::GenerateBinaryOperatorCall(intptr_t node_id,
                                               intptr_t token_index,
                                               const char* name) {
  const String& operator_name = String::ZoneHandle(String::NewSymbol(name));
  const int kNumberOfArguments = 2;
  const Array& kNoArgumentNames = Array::Handle();
  GenerateInstanceCall(node_id,
                       token_index,
                       operator_name,
                       kNumberOfArguments,
                       kNoArgumentNames);
}


void CodeGenerator::VisitBinaryOpNode(BinaryOpNode* node) {
  if ((node->kind() == Token::kAND) || (node->kind() == Token::kOR)) {
    // Operators "&&" and "||" cannot be overloaded, therefore inline them
    // instead of calling the operator.
    GenerateLogicalAndOrOp(node);
    return;
  }
  node->left()->Visit(this);
  node->right()->Visit(this);
  MarkDeoptPoint(node->id(), node->token_index());
  GenerateBinaryOperatorCall(node->id(), node->token_index(), node->Name());
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


void CodeGenerator::VisitStringConcatNode(StringConcatNode* node) {
  const String& cls_name = String::Handle(String::NewSymbol("StringBase"));
  const Library& core_lib = Library::Handle(
      Isolate::Current()->object_store()->core_library());
  const Class& cls = Class::Handle(core_lib.LookupClass(cls_name));
  ASSERT(!cls.IsNull());
  const String& func_name = String::Handle(String::NewSymbol("_interpolate"));
  const int number_of_parameters = 1;
  const Function& interpol_func = Function::ZoneHandle(
      Resolver::ResolveStatic(cls, func_name,
                              number_of_parameters,
                              Array::Handle(),
                              Resolver::kIsQualified));
  ASSERT(!interpol_func.IsNull());

  // First try to concatenate and canonicalize the values at compile time.
  bool compile_time_interpolation = true;
  Array& literals = Array::Handle(Array::New(node->values()->length()));
  for (int i = 0; i < node->values()->length(); i++) {
    if (node->values()->ElementAt(i)->IsLiteralNode()) {
      LiteralNode* lit = node->values()->ElementAt(i)->AsLiteralNode();
      literals.SetAt(i, lit->literal());
    } else {
      compile_time_interpolation = false;
      break;
    }
  }
  if (compile_time_interpolation) {
    if (!IsResultNeeded(node)) {
      return;
    }
    // Build argument array to pass to the interpolation function.
    GrowableArray<const Object*> interpolate_arg;
    interpolate_arg.Add(&literals);
    const Array& kNoArgumentNames = Array::Handle();
    // Call the interpolation function.
    String& concatenated = String::ZoneHandle();
    concatenated ^= DartEntry::InvokeStatic(interpol_func,
                                            interpolate_arg,
                                            kNoArgumentNames);
    if (concatenated.IsUnhandledException()) {
      ErrorMsg(node->token_index(),
          "Exception thrown in CodeGenerator::VisitStringConcatNode");
    }
    ASSERT(!concatenated.IsNull());
    concatenated = String::NewSymbol(concatenated);

    __ LoadObject(EAX, concatenated);
    __ pushl(EAX);
    return;
  }

  // Could not concatenate at compile time, generate a call to
  // interpolation function.
  ArgumentListNode* interpol_arg = new ArgumentListNode(node->token_index());
  interpol_arg->Add(node->values());
  node->values()->Visit(this);
  __ LoadObject(ECX, interpol_func);
  __ LoadObject(EDX, ArgumentsDescriptor(interpol_arg->length(),
                                         interpol_arg->names()));
  GenerateCall(node->token_index(), &StubCode::CallStaticFunctionLabel());
  __ addl(ESP, Immediate(interpol_arg->length() * kWordSize));
  // Result is in EAX.
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


void CodeGenerator::VisitInstanceCallNode(InstanceCallNode* node) {
  const int number_of_arguments = node->arguments()->length() + 1;
  // Compute the receiver object and pass it as first argument to call.
  node->receiver()->Visit(this);
  // Now compute rest of the arguments to the call.
  node->arguments()->Visit(this);
  // Some method may be inlined using type feedback, therefore this may be a
  // deoptimization point.
  MarkDeoptPoint(node->id(), node->token_index());

  GenerateInstanceCall(node->id(),
                       node->token_index(),
                       node->function_name(),
                       number_of_arguments,
                       node->arguments()->names());
  // Result is in EAX.
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


void CodeGenerator::VisitStaticCallNode(StaticCallNode* node) {
  node->arguments()->Visit(this);
  __ LoadObject(ECX, node->function());
  __ LoadObject(EDX, ArgumentsDescriptor(node->arguments()->length(),
                                         node->arguments()->names()));
  GenerateCall(node->token_index(), &StubCode::CallStaticFunctionLabel());
  __ addl(ESP, Immediate(node->arguments()->length() * kWordSize));
  // Result is in EAX.
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


void CodeGenerator::VisitClosureCallNode(ClosureCallNode* node) {
  // The spec states that the closure is evaluated before the arguments.
  // Preserve the current context, since it will be overridden by the closure
  // context during the call.
  __ pushl(CTX);
  // Compute the closure object and pass it as first argument to the stub.
  node->closure()->Visit(this);
  // Now compute the arguments to the call.
  node->arguments()->Visit(this);
  // Set up the number of arguments (excluding the closure) to the ClosureCall
  // stub which will setup the closure context and jump to the entrypoint of the
  // closure function (the function will be compiled if it has not already been
  // compiled).
  // NOTE: The stub accesses the closure before the parameter list.
  __ LoadObject(EDX, ArgumentsDescriptor(node->arguments()->length(),
                                         node->arguments()->names()));
  GenerateCall(node->token_index(), &StubCode::CallClosureFunctionLabel());
  __ addl(ESP, Immediate((node->arguments()->length() + 1) * kWordSize));
  // Restore the context.
  __ popl(CTX);
  // Result is in EAX.
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


// Pushes the type arguments of the instantiator on the stack.
void CodeGenerator::GenerateInstantiatorTypeArguments(intptr_t token_index) {
  Class& instantiator_class = Class::Handle();
  Function& outer_function =
      Function::Handle(parsed_function().function().raw());
  while (outer_function.IsLocalFunction()) {
    outer_function = outer_function.parent_function();
  }
  if (outer_function.IsFactory()) {
    instantiator_class = outer_function.signature_class();
  } else {
    instantiator_class = outer_function.owner();
  }
  if (instantiator_class.NumTypeParameters() == 0) {
    // The type arguments are compile time constants.
    AbstractTypeArguments& type_arguments = AbstractTypeArguments::ZoneHandle();
    // TODO(regis): Temporary type should be allocated in new gen heap.
    Type& type = Type::Handle(
        Type::NewParameterizedType(instantiator_class, type_arguments));
    String& errmsg = String::Handle();
    type = ClassFinalizer::FinalizeAndCanonicalizeType(type, &errmsg);
    if (!errmsg.IsNull()) {
      ErrorMsg(token_index, errmsg.ToCString());
    }
    type_arguments = type.arguments();
    __ PushObject(type_arguments);
  } else {
    ASSERT(parsed_function().instantiator() != NULL);
    parsed_function().instantiator()->Visit(this);
    if (!outer_function.IsFactory()) {
      __ popl(EAX);  // Pop instantiator.
      // The instantiator is the receiver of the caller, which is not a factory.
      // The receiver cannot be null; extract its AbstractTypeArguments object.
      // Note that in the factory case, the instantiator is the first parameter
      // of the factory, i.e. already an AbstractTypeArguments object.
      intptr_t type_arguments_instance_field_offset =
          instantiator_class.type_arguments_instance_field_offset();
      ASSERT(type_arguments_instance_field_offset != Class::kNoTypeArguments);
      __ movl(EAX, FieldAddress(EAX, type_arguments_instance_field_offset));
      __ pushl(EAX);
    }
  }
}


// Pushes the type arguments on the stack in preparation of a constructor or
// factory call.
// For a factory call, instantiates (possibly requiring an additional run time
// call) and pushes the type argument vector that will be passed as implicit
// first parameter to the factory.
// For a constructor call allocating an object of a parameterized class, pushes
// the type arguments and the type arguments of the instantiator, without ever
// generating an additional run time call.
// Does nothing for a constructor call allocating an object of a non
// parameterized class.
// Note that a class without proper type parameters may still be parameterized,
// e.g. class A extends Array<int>.
void CodeGenerator::GenerateTypeArguments(ConstructorCallNode* node,
                                          bool requires_type_arguments) {
  const Immediate raw_null =
      Immediate(reinterpret_cast<intptr_t>(Object::null()));
  // Instantiate the type arguments if necessary.
  if (node->type_arguments().IsNull() ||
      node->type_arguments().IsInstantiated()) {
    if (requires_type_arguments) {
      // A factory requires the type arguments as first parameter.
      __ PushObject(node->type_arguments());
      if (!node->constructor().IsFactory()) {
        // The allocator additionally requires the instantiator type arguments.
        __ pushl(raw_null);  // Null instantiator.
      }
    }
  } else {
    // The type arguments are uninstantiated.
    ASSERT(requires_type_arguments);
    GenerateInstantiatorTypeArguments(node->token_index());
    __ popl(EAX);  // Pop instantiator.
    // EAX is the instantiator AbstractTypeArguments object (or null).
    // If EAX is null, no need to instantiate the type arguments, use null, and
    // allocate an object of a raw type.
    Label type_arguments_instantiated, type_arguments_uninstantiated;
    __ cmpl(EAX, raw_null);
    __ j(EQUAL, &type_arguments_instantiated, Assembler::kNearJump);

    // Instantiate non-null type arguments.
    if (node->type_arguments().IsUninstantiatedIdentity()) {
      // Check if the instantiator type argument vector is a TypeArguments of a
      // matching length and, if so, use it as the instantiated type_arguments.
      __ LoadObject(ECX, Class::ZoneHandle(Object::type_arguments_class()));
      __ cmpl(ECX, FieldAddress(EAX, Object::class_offset()));
      __ j(NOT_EQUAL, &type_arguments_uninstantiated, Assembler::kNearJump);
      Immediate arguments_length = Immediate(reinterpret_cast<int32_t>(
          Smi::New(node->type_arguments().Length())));
      __ cmpl(FieldAddress(EAX, TypeArguments::length_offset()),
          arguments_length);
      __ j(EQUAL, &type_arguments_instantiated, Assembler::kNearJump);
    }
    __ Bind(&type_arguments_uninstantiated);
    if (node->constructor().IsFactory()) {
      // A runtime call to instantiate the type arguments is required before
      // calling the factory.
      const Object& result = Object::ZoneHandle();
      __ PushObject(result);  // Make room for the result of the runtime call.
      __ PushObject(node->type_arguments());
      __ pushl(EAX);  // Push instantiator type arguments.
      GenerateCallRuntime(node->id(),
                          node->token_index(),
                          kInstantiateTypeArgumentsRuntimeEntry);
      __ popl(EAX);  // Pop instantiator type arguments.
      __ popl(EAX);  // Pop uninstantiated type arguments.
      __ popl(EAX);  // Pop instantiated type arguments.
      __ Bind(&type_arguments_instantiated);
      __ pushl(EAX);  // Instantiated type arguments.
    } else {
      // In the non-factory case, we rely on the allocation stub to
      // instantiate the type arguments.
      __ PushObject(node->type_arguments());
      __ pushl(EAX);  // Instantiator type arguments.
      Label type_arguments_pushed;
      __ jmp(&type_arguments_pushed, Assembler::kNearJump);

      __ Bind(&type_arguments_instantiated);
      __ pushl(EAX);  // Instantiated type arguments.
      __ pushl(raw_null);  // Null instantiator.
      __ Bind(&type_arguments_pushed);
    }
  }
}


void CodeGenerator::VisitConstructorCallNode(ConstructorCallNode* node) {
  if (node->constructor().IsFactory()) {
    const bool requires_type_arguments = true;  // Always first arg to factory.
    GenerateTypeArguments(node, requires_type_arguments);
    // The top of stack is an instantiated AbstractTypeArguments object
    // (or null).
    int num_args = node->arguments()->length() + 1;  // +1 to include type args.
    node->arguments()->Visit(this);
    // Call the factory.
    __ LoadObject(ECX, node->constructor());
    __ LoadObject(EDX, ArgumentsDescriptor(num_args,
                                           node->arguments()->names()));
    GenerateCall(node->token_index(), &StubCode::CallStaticFunctionLabel());
    // Factory constructor returns object in EAX.
    __ addl(ESP, Immediate(num_args * kWordSize));
    if (IsResultNeeded(node)) {
      __ pushl(EAX);
    }
    return;
  }

  const Class& cls = Class::ZoneHandle(node->constructor().owner());
  const bool requires_type_arguments = cls.HasTypeArguments();
  GenerateTypeArguments(node, requires_type_arguments);

  // If cls is parameterized, the type arguments and the instantiator's
  // type arguments are on the stack.
  const Code& stub = Code::Handle(StubCode::GetAllocationStubForClass(cls));
  const ExternalLabel label(cls.ToCString(), stub.EntryPoint());
  GenerateCall(node->token_index(), &label);
  if (requires_type_arguments) {
    __ popl(ECX);  // Pop type arguments.
    __ popl(ECX);  // Pop instantiator type arguments.
  }

  if (IsResultNeeded(node)) {
    __ pushl(EAX);  // Set up return value from allocate.
  }

  // First argument(this) for constructor call which follows.
  __ pushl(EAX);
  // Second argument is the implicit construction phase parameter.
  // Run both the constructor initializer list and the constructor body.
  __ PushObject(Smi::ZoneHandle(Smi::New(Function::kCtorPhaseAll)));


  // Now setup rest of the arguments for the constructor call.
  node->arguments()->Visit(this);

  // Call the constructor.
  // +2 to include implicit receiver and phase arguments.
  int num_args = node->arguments()->length() + 2;
  __ LoadObject(ECX, node->constructor());
  __ LoadObject(EDX, ArgumentsDescriptor(num_args, node->arguments()->names()));
  GenerateCall(node->token_index(), &StubCode::CallStaticFunctionLabel());
  // Constructors do not return any value.

  // Pop out all the other arguments on the stack.
  __ addl(ESP, Immediate(num_args * kWordSize));
}


// Expects receiver on stack, returns result in EAX..
void CodeGenerator::GenerateInstanceGetterCall(intptr_t node_id,
                                               intptr_t token_index,
                                               const String& field_name) {
  const String& getter_name = String::ZoneHandle(Field::GetterName(field_name));
  const int kNumberOfArguments = 1;
  const Array& kNoArgumentNames = Array::Handle();
  GenerateInstanceCall(node_id,
                       token_index,
                       getter_name,
                       kNumberOfArguments,
                       kNoArgumentNames);
}


// Call to the instance getter.
void CodeGenerator::VisitInstanceGetterNode(InstanceGetterNode* node) {
  node->receiver()->Visit(this);
  MarkDeoptPoint(node->id(), node->token_index());
  GenerateInstanceGetterCall(node->id(),
                             node->token_index(),
                             node->field_name());
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


// Expects receiver and value on stack.
void CodeGenerator::GenerateInstanceSetterCall(intptr_t node_id,
                                               intptr_t token_index,
                                               const String& field_name) {
  const String& setter_name = String::ZoneHandle(Field::SetterName(field_name));
  const int kNumberOfArguments = 2;  // receiver + value.
  const Array& kNoArgumentNames = Array::Handle();
  GenerateInstanceCall(node_id,
                       token_index,
                       setter_name,
                       kNumberOfArguments,
                       kNoArgumentNames);
}


// The call to the instance setter implements the assignment to a field.
// The result of the assignment to a field is the value being stored.
void CodeGenerator::VisitInstanceSetterNode(InstanceSetterNode* node) {
  // Compute the receiver object and pass it as first argument to call.
  node->receiver()->Visit(this);
  node->value()->Visit(this);
  MarkDeoptPoint(node->id(), node->token_index());
  if (IsResultNeeded(node)) {
    __ popl(EAX);   // value.
    __ popl(EDX);   // receiver.
    __ pushl(EAX);  // Preserve value.
    __ pushl(EDX);  // arg0: receiver.
    __ pushl(EAX);  // arg1: value.
  }
  // It is not necessary to generate a type test of the assigned value here,
  // because the setter will check the type of its incoming arguments.
  GenerateInstanceSetterCall(node->id(),
                             node->token_index(),
                             node->field_name());
}


// Return result in EAX.
void CodeGenerator::GenerateStaticGetterCall(intptr_t token_index,
                                             const Class& field_class,
                                             const String& field_name) {
  const String& getter_name = String::Handle(Field::GetterName(field_name));
  const Function& function =
      Function::ZoneHandle(field_class.LookupStaticFunction(getter_name));
  if (function.IsNull()) {
    ErrorMsg(token_index, "Static getter does not exist: %s",
        getter_name.ToCString());
  }
  __ LoadObject(ECX, function);
  const int kNumberOfArguments = 0;
  const Array& kNoArgumentNames = Array::Handle();
  __ LoadObject(EDX, ArgumentsDescriptor(kNumberOfArguments, kNoArgumentNames));
  GenerateCall(token_index, &StubCode::CallStaticFunctionLabel());
  // No arguments were pushed, hence nothing to pop.
}


// Call to static getter.
void CodeGenerator::VisitStaticGetterNode(StaticGetterNode* node) {
  GenerateStaticGetterCall(node->token_index(),
                           node->cls(),
                           node->field_name());
  // Result is in EAX.
  if (IsResultNeeded(node)) {
    __ pushl(EAX);
  }
}


// Expects value on stack.
void CodeGenerator::GenerateStaticSetterCall(intptr_t token_index,
                                             const Class& field_class,
                                             const String& field_name) {
  const String& setter_name = String::Handle(Field::SetterName(field_name));
  const Function& function =
      Function::ZoneHandle(field_class.LookupStaticFunction(setter_name));
  __ LoadObject(ECX, function);
  const int kNumberOfArguments = 1;  // value.
  const Array& kNoArgumentNames = Array::Handle();
  __ LoadObject(EDX, ArgumentsDescriptor(kNumberOfArguments, kNoArgumentNames));
  GenerateCall(token_index, &StubCode::CallStaticFunctionLabel());
  __ addl(ESP, Immediate(kNumberOfArguments * kWordSize));
}


// The call to static setter implements assignment to a static field.
// The result of the assignment is the value being stored.
void CodeGenerator::VisitStaticSetterNode(StaticSetterNode* node) {
  node->value()->Visit(this);
  if (IsResultNeeded(node)) {
    // Preserve the original value when returning from setter.
    __ movl(EAX, Address(ESP, 0));
    __ pushl(EAX);  // arg0: value.
  }
  // It is not necessary to generate a type test of the assigned value here,
  // because the setter will check the type of its incoming arguments.
  GenerateStaticSetterCall(node->token_index(),
                           node->cls(),
                           node->field_name());
}


void CodeGenerator::VisitNativeBodyNode(NativeBodyNode* node) {
  // Push the result place holder initialized to NULL.
  __ PushObject(Object::ZoneHandle());
  // Pass a pointer to the first argument in EAX.
  if (!node->has_optional_parameters()) {
    __ leal(EAX, Address(EBP, (1 + node->argument_count()) * kWordSize));
  } else {
    __ leal(EAX, Address(EBP, -1 * kWordSize));
  }
  __ movl(ECX, Immediate(reinterpret_cast<uword>(node->native_c_function())));
  __ movl(EDX, Immediate(node->argument_count()));
  GenerateCall(node->token_index(), &StubCode::CallNativeCFunctionLabel());
  // Result is on the stack.
  if (!IsResultNeeded(node)) {
    __ popl(EAX);
  }
}


void CodeGenerator::VisitCatchClauseNode(CatchClauseNode* node) {
  // NOTE: The implicit variables ':saved_context', ':exception_var'
  // and ':stacktrace_var' can never be captured variables.
  // Restore CTX from local variable ':saved_context'.
  GenerateLoadVariable(CTX, node->context_var());

  // Restore ESP from EBP as we are coming from a throw and the code for
  // popping arguments has not been run.
  ASSERT(locals_space_size() >= 0);
  __ movl(ESP, EBP);
  __ subl(ESP, Immediate(locals_space_size()));

  // The JumpToExceptionHandler trampoline code sets up
  // - the exception object in EAX (kExceptionObjectReg)
  // - the stacktrace object in register EDX (kStackTraceObjectReg)
  // We now setup the exception object and the trace object
  // so that the handler code has access to these objects.
  GenerateStoreVariable(node->exception_var(),
                        kExceptionObjectReg,
                        kNoRegister);
  GenerateStoreVariable(node->stacktrace_var(),
                        kStackTraceObjectReg,
                        kNoRegister);

  // Now generate code for the catch handler block.
  node->VisitChildren(this);
}


void CodeGenerator::VisitTryCatchNode(TryCatchNode* node) {
  CodeGeneratorState codegen_state(this);
  int outer_try_index = state()->try_index();
  // We are about to generate code for a new try block, generate an
  // unique 'try index' for this block and set that try index in
  // the code generator state.
  int try_index = generate_next_try_index();
  state()->set_try_index(try_index);
  exception_handlers_list_->AddHandler(try_index, -1);

  // Preserve CTX into local variable '%saved_context'.
  GenerateStoreVariable(node->context_var(), CTX, kNoRegister);

  node->try_block()->Visit(this);

  // We are done generating code for the try block.
  ASSERT(state()->try_index() > CatchClauseNode::kInvalidTryIndex);
  ASSERT(try_index == state()->try_index());
  state()->set_try_index(outer_try_index);

  CatchClauseNode* catch_block = node->catch_block();
  if (catch_block != NULL) {
    // Jump over the catch handler block, when exceptions are thrown we
    // will end up at the next instruction.
    __ jmp(node->end_catch_label()->continue_label());

    // Set the corresponding try index for this catch block so
    // that we can set the appropriate handler pc when we generate
    // code for this catch block.
    catch_block->set_try_index(try_index);

    // Set the handler pc for this try index in the exception handler
    // table.
    exception_handlers_list_->SetPcOffset(try_index, assembler_->CodeSize());

    // Generate code for the catch block.
    catch_block->Visit(this);

    // Bind the end of catch blocks label here.
    __ Bind(node->end_catch_label()->continue_label());
  }

  // Generate code for the finally block if one exists.
  if (node->finally_block() != NULL) {
    node->finally_block()->Visit(this);
  }
}


void CodeGenerator::VisitThrowNode(ThrowNode* node) {
  const Object& result = Object::ZoneHandle();
  node->exception()->Visit(this);
  __ popl(EAX);  // Exception object is now in EAX.
  if (node->stacktrace() != NULL) {
    __ PushObject(result);  // Make room for the result of the runtime call.
    __ pushl(EAX);  // Push the exception object.
    node->stacktrace()->Visit(this);
    GenerateCallRuntime(node->id(), node->token_index(), kReThrowRuntimeEntry);
  } else {
    __ PushObject(result);  // Make room for the result of the runtime call.
    __ pushl(EAX);  // Push the exception object.
    GenerateCallRuntime(node->id(), node->token_index(), kThrowRuntimeEntry);
  }
  // We should never return here.
  __ int3();
}


void CodeGenerator::VisitInlinedFinallyNode(InlinedFinallyNode* node) {
  int try_index = state()->try_index();
  if (try_index >= 0) {
    // We are about to generate code for an inlined finally block. Exceptions
    // thrown in this block of code should be treated as though they are
    // thrown not from the current try block but the outer try block if any.
    // the code generator state.
    state()->set_try_index((try_index - 1));
  }

  // Restore CTX from local variable ':saved_context'.
  GenerateLoadVariable(CTX, node->context_var());
  node->finally_block()->Visit(this);

  if (try_index >= 0) {
    state()->set_try_index(try_index);
  }
}


void CodeGenerator::GenerateCall(intptr_t token_index,
                                 const ExternalLabel* ext_label) {
  __ call(ext_label);
  AddCurrentDescriptor(PcDescriptors::kOther, AstNode::kNoId, token_index);
}


void CodeGenerator::GenerateCallRuntime(intptr_t node_id,
                                        intptr_t token_index,
                                        const RuntimeEntry& entry) {
  __ CallRuntimeFromDart(entry);
  AddCurrentDescriptor(PcDescriptors::kOther, node_id, token_index);
}


void CodeGenerator::MarkDeoptPoint(intptr_t node_id,
                                   intptr_t token_index) {
  ASSERT(node_id != AstNode::kNoId);
  AddCurrentDescriptor(PcDescriptors::kDeopt, node_id, token_index);
}


// Uses current pc position and try-index.
void CodeGenerator::AddCurrentDescriptor(PcDescriptors::Kind kind,
                                         intptr_t node_id,
                                         intptr_t token_index) {
  pc_descriptors_list_->AddDescriptor(kind,
                                      assembler_->CodeSize(),
                                      node_id,
                                      token_index,
                                      state()->try_index());
}


void CodeGenerator::ErrorMsg(intptr_t token_index, const char* format, ...) {
  const intptr_t kMessageBufferSize = 512;
  char message_buffer[kMessageBufferSize];
  va_list args;
  va_start(args, format);
  const Class& cls = Class::Handle(parsed_function_.function().owner());
  const Script& script = Script::Handle(cls.script());
  Parser::FormatMessage(script, token_index, "Error",
                        message_buffer, kMessageBufferSize,
                        format, args);
  va_end(args);
  Isolate::Current()->long_jump_base()->Jump(1, message_buffer);
  UNREACHABLE();
}

}  // namespace dart

#endif  // defined TARGET_ARCH_IA32
