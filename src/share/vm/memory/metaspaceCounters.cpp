/*
 * Copyright (c) 2012, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#include "precompiled.hpp"
#include "memory/metaspaceCounters.hpp"
#include "memory/resourceArea.hpp"
#include "runtime/globals.hpp"
#include "runtime/perfData.hpp"
#include "utilities/exceptions.hpp"

class MetaspacePerfCounters: public CHeapObj<mtInternal> {
  friend class VMStructs;
  PerfVariable*      _capacity;
  PerfVariable*      _used;
  PerfVariable*      _max_capacity;

  PerfVariable* create_variable(const char *ns, const char *name, size_t value, TRAPS) {
    const char *path = PerfDataManager::counter_name(ns, name);
    return PerfDataManager::create_variable(SUN_GC, path, PerfData::U_Bytes, value, THREAD);
  }

  void create_constant(const char *ns, const char *name, size_t value, TRAPS) {
    const char *path = PerfDataManager::counter_name(ns, name);
    PerfDataManager::create_constant(SUN_GC, path, PerfData::U_Bytes, value, THREAD);
  }

 public:
  MetaspacePerfCounters(const char* ns, size_t min_capacity, size_t curr_capacity, size_t max_capacity, size_t used) {
    EXCEPTION_MARK;
    ResourceMark rm;

    create_constant(ns, "minCapacity", min_capacity, THREAD);
    _capacity = create_variable(ns, "capacity", curr_capacity, THREAD);
    _max_capacity = create_variable(ns, "maxCapacity", max_capacity, THREAD);
    _used = create_variable(ns, "used", used, THREAD);
  }

  void update(size_t capacity, size_t max_capacity, size_t used) {
    _capacity->set_value(capacity);
    _max_capacity->set_value(max_capacity);
    _used->set_value(used);
  }
};

MetaspacePerfCounters* MetaspaceCounters::_perf_counters = NULL;

size_t MetaspaceCounters::calculate_capacity() {
  // The total capacity is the sum of
  //   1) capacity of Metachunks in use by all Metaspaces
  //   2) unused space at the end of each Metachunk
  //   3) space in the freelist
  size_t total_capacity = MetaspaceAux::allocated_capacity_bytes()
    + MetaspaceAux::free_bytes() + MetaspaceAux::free_chunks_total_in_bytes();
  return total_capacity;
}

void MetaspaceCounters::initialize_performance_counters() {
  if (UsePerfData) {
    assert(_perf_counters == NULL, "Should only be initialized once");

    size_t min_capacity = MetaspaceAux::min_chunk_size();
    size_t capacity = calculate_capacity();
    size_t max_capacity = MetaspaceAux::reserved_in_bytes();
    size_t used = MetaspaceAux::allocated_used_bytes();

    _perf_counters = new MetaspacePerfCounters("metaspace", min_capacity, capacity, max_capacity, used);
  }
}

void MetaspaceCounters::update_performance_counters() {
  if (UsePerfData) {
    assert(_perf_counters != NULL, "Should be initialized");

    size_t capacity = calculate_capacity();
    size_t max_capacity = MetaspaceAux::reserved_in_bytes();
    size_t used = MetaspaceAux::allocated_used_bytes();

    _perf_counters->update(capacity, max_capacity, used);
  }
}

MetaspacePerfCounters* CompressedClassSpaceCounters::_perf_counters = NULL;

size_t CompressedClassSpaceCounters::calculate_capacity() {
    return MetaspaceAux::allocated_capacity_bytes(_class_type) +
           MetaspaceAux::free_bytes(_class_type) +
           MetaspaceAux::free_chunks_total_in_bytes(_class_type);
}

void CompressedClassSpaceCounters::update_performance_counters() {
  if (UsePerfData && UseCompressedKlassPointers) {
    assert(_perf_counters != NULL, "Should be initialized");

    size_t capacity = calculate_capacity();
    size_t max_capacity = MetaspaceAux::reserved_in_bytes(_class_type);
    size_t used = MetaspaceAux::allocated_used_bytes(_class_type);

    _perf_counters->update(capacity, max_capacity, used);
  }
}

void CompressedClassSpaceCounters::initialize_performance_counters() {
  if (UsePerfData) {
    assert(_perf_counters == NULL, "Should only be initialized once");
    const char* ns = "compressedclassspace";

    if (UseCompressedKlassPointers) {
      size_t min_capacity = MetaspaceAux::min_chunk_size();
      size_t capacity = calculate_capacity();
      size_t max_capacity = MetaspaceAux::reserved_in_bytes(_class_type);
      size_t used = MetaspaceAux::allocated_used_bytes(_class_type);

      _perf_counters = new MetaspacePerfCounters(ns, min_capacity, capacity, max_capacity, used);
    } else {
      _perf_counters = new MetaspacePerfCounters(ns, 0, 0, 0, 0);
    }
  }
}
