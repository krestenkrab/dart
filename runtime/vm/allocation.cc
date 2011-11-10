// Copyright (c) 2011, the Dart project authors.  Please see the AUTHORS file
// for details. All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#include "vm/allocation.h"

#include "vm/assert.h"
#include "vm/isolate.h"
#include "vm/zone.h"

namespace dart {

StackResource::StackResource() {
  isolate_ = Isolate::Current();
  previous_ = isolate_->top_resource();
  isolate_->set_top_resource(this);
}


StackResource::~StackResource() {
  StackResource* top = isolate_->top_resource();
  ASSERT(top == this);
  isolate_->set_top_resource(previous_);
}

ZoneAllocated::~ZoneAllocated() {
  UNREACHABLE();
}

void* ZoneAllocated::operator new(uword size) {
  Isolate* isolate = Isolate::Current();
  ASSERT(isolate != NULL);
  ASSERT(isolate->current_zone() != NULL);
  return reinterpret_cast<void*>(isolate->current_zone()->Allocate(size));
}

}  // namespace dart
