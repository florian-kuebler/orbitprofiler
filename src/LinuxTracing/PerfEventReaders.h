// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LINUX_TRACING_PERF_EVENT_READERS_H_
#define LINUX_TRACING_PERF_EVENT_READERS_H_

#include <linux/perf_event.h>
#include <stdint.h>
#include <sys/types.h>

#include "PerfEvent.h"
#include "PerfEventRecords.h"
#include "PerfEventRingBuffer.h"

namespace orbit_linux_tracing {

// Helper functions for reads from a perf_event_open ring buffer that require
// more complex operations than simply copying an entire perf_event_open record.

// This function reads sample_id, which is always the last field
// in the perf event record unless it is PERF_RECORD_SAMPLE.
void ReadPerfSampleIdAll(PerfEventRingBuffer* ring_buffer, const perf_event_header& header,
                         perf_event_sample_id_tid_time_streamid_cpu* sample_id);

uint64_t ReadSampleRecordTime(PerfEventRingBuffer* ring_buffer);

uint64_t ReadSampleRecordStreamId(PerfEventRingBuffer* ring_buffer);

pid_t ReadSampleRecordPid(PerfEventRingBuffer* ring_buffer);

uint64_t ReadThrottleUnthrottleRecordTime(PerfEventRingBuffer* ring_buffer);

MmapPerfEvent ConsumeMmapPerfEvent(PerfEventRingBuffer* ring_buffer,
                                   const perf_event_header& header);

template <typename T>
T ConsumeStackSamplePerfEvent(PerfEventRingBuffer* ring_buffer, const perf_event_header& header,
                              uint64_t* stream_id) {
  // We expect the following layout of the perf event:
  //  struct {
  //    struct perf_event_header header;
  //    u64 sample_id;          /* if PERF_SAMPLE_IDENTIFIER */
  //    u32 pid, tid;           /* if PERF_SAMPLE_TID */
  //    u64 time;               /* if PERF_SAMPLE_TIME */
  //    u64 stream_id;          /* if PERF_SAMPLE_STREAM_ID */
  //    u32 cpu, res;           /* if PERF_SAMPLE_CPU */
  //    u64 abi;                /* if PERF_SAMPLE_REGS_USER */
  //    u64 regs[weight(mask)]; /* if PERF_SAMPLE_REGS_USER */
  //    u64 size;               /* if PERF_SAMPLE_STACK_USER */
  //    char data[size];        /* if PERF_SAMPLE_STACK_USER */
  //    u64 dyn_size;           /* if PERF_SAMPLE_STACK_USER && size != 0 */
  //  };
  // Unfortunately, the value of `size` is not constant, so we need to compute the offsets by hand,
  // rather than relying on a struct.

  size_t offset_of_size =
      offsetof(perf_event_stack_sample_fixed, regs) + sizeof(perf_event_sample_regs_user_all);
  size_t offset_of_data = offset_of_size + sizeof(uint64_t);

  uint64_t size = 0;
  ring_buffer->ReadValueAtOffset(&size, offset_of_size);

  size_t offset_of_dyn_size = offset_of_data + (size * sizeof(char));

  uint64_t dyn_size = 0;
  ring_buffer->ReadValueAtOffset(&dyn_size, offset_of_dyn_size);

  perf_event_sample_id_tid_time_streamid_cpu sample_id;
  ring_buffer->ReadValueAtOffset(&sample_id, offsetof(perf_event_stack_sample_fixed, sample_id));

  if (stream_id != nullptr) {
    *stream_id = sample_id.stream_id;
  }

  T event{
      .timestamp = sample_id.time,
      .ordered_stream = PerfEventOrderedStream::FileDescriptor(ring_buffer->GetFileDescriptor()),
      .data =
          StackSamplePerfEventData{
              .pid = static_cast<pid_t>(sample_id.pid),
              .tid = static_cast<pid_t>(sample_id.tid),
              .regs = make_unique_for_overwrite<perf_event_sample_regs_user_all>(),
              .dyn_size = dyn_size,
              .data = make_unique_for_overwrite<char[]>(dyn_size),
          },
  };

  ring_buffer->ReadValueAtOffset(event.data.regs.get(),
                                 offsetof(perf_event_stack_sample_fixed, regs));
  ring_buffer->ReadRawAtOffset(event.data.data.get(), offset_of_data, dyn_size);
  ring_buffer->SkipRecord(header);
  return event;
}

template <typename T>
T ConsumeCallchainSamplePerfEvent(PerfEventRingBuffer* ring_buffer, const perf_event_header& header,
                                  uint64_t* stream_id) {
  // We expect the following layout of the perf event:
  //  struct {
  //    struct perf_event_header header;
  //    u64 sample_id;          /* if PERF_SAMPLE_IDENTIFIER */
  //    u32 pid, tid;           /* if PERF_SAMPLE_TID */
  //    u64 time;               /* if PERF_SAMPLE_TIME */
  //    u64 stream_id;          /* if PERF_SAMPLE_STREAM_ID */
  //    u32 cpu, res;           /* if PERF_SAMPLE_CPU */
  //    u64 nr;                 /* if PERF_SAMPLE_CALLCHAIN */
  //    u64 ips[nr];            /* if PERF_SAMPLE_CALLCHAIN */
  //    u64 abi;                /* if PERF_SAMPLE_REGS_USER */
  //    u64 regs[weight(mask)]; /* if PERF_SAMPLE_REGS_USER */
  //    u64 size;               /* if PERF_SAMPLE_STACK_USER */
  //    char data[size];        /* if PERF_SAMPLE_STACK_USER */
  //    u64 dyn_size;           /* if PERF_SAMPLE_STACK_USER && size != 0 */
  //  };
  // Unfortunately, the number of `ips` is dynamic, so we need to compute the offsets by hand,
  // rather than relying on a struct.
  uint64_t nr = 0;
  ring_buffer->ReadValueAtOffset(&nr, offsetof(perf_event_callchain_sample_fixed, nr));

  const uint64_t size_of_ips_in_bytes = nr * sizeof(uint64_t);

  const size_t offset_of_ips = offsetof(perf_event_callchain_sample_fixed, nr) +
                               sizeof(perf_event_callchain_sample_fixed::nr);
  const size_t offset_of_regs_user_struct = offset_of_ips + size_of_ips_in_bytes;
  // Note that perf_event_sample_regs_user_all contains abi and the regs array.
  const size_t offset_of_size =
      offset_of_regs_user_struct + sizeof(perf_event_sample_regs_user_all);
  const size_t offset_of_data = offset_of_size + sizeof(uint64_t);

  uint64_t size = 0;
  ring_buffer->ReadRawAtOffset(&size, offset_of_size, sizeof(uint64_t));

  const size_t offset_of_dyn_size = offset_of_data + size;
  uint64_t dyn_size = 0;
  ring_buffer->ReadRawAtOffset(&dyn_size, offset_of_dyn_size, sizeof(uint64_t));
  perf_event_sample_id_tid_time_streamid_cpu sample_id;
  ring_buffer->ReadValueAtOffset(&sample_id,
                                 offsetof(perf_event_callchain_sample_fixed, sample_id));
  if (stream_id != nullptr) {
    *stream_id = sample_id.stream_id;
  }

  T event{
      .timestamp = sample_id.time,
      .ordered_stream = PerfEventOrderedStream::FileDescriptor(ring_buffer->GetFileDescriptor()),
      .data =
          CallchainSamplePerfEventData{
              .pid = static_cast<pid_t>(sample_id.pid),
              .tid = static_cast<pid_t>(sample_id.tid),
              .ips_size = nr,
              .ips = make_unique_for_overwrite<uint64_t[]>(nr),
              .regs = make_unique_for_overwrite<perf_event_sample_regs_user_all>(),
              .data = make_unique_for_overwrite<char[]>(dyn_size),
          },
  };

  ring_buffer->ReadRawAtOffset(event.data.ips.get(), offset_of_ips, size_of_ips_in_bytes);
  ring_buffer->ReadRawAtOffset(event.data.regs.get(), offset_of_regs_user_struct,
                               sizeof(perf_event_sample_regs_user_all));
  ring_buffer->ReadRawAtOffset(event.data.data.get(), offset_of_data, dyn_size);

  ring_buffer->SkipRecord(header);
  return event;
}

GenericTracepointPerfEvent ConsumeGenericTracepointPerfEvent(PerfEventRingBuffer* ring_buffer,
                                                             const perf_event_header& header);

SchedWakeupPerfEvent ConsumeSchedWakeupPerfEvent(PerfEventRingBuffer* ring_buffer,
                                                 const perf_event_header& header);

AmdgpuCsIoctlPerfEvent ConsumeAmdgpuCsIoctlPerfEvent(PerfEventRingBuffer* ring_buffer,
                                                     const perf_event_header& header);

AmdgpuSchedRunJobPerfEvent ConsumeAmdgpuSchedRunJobPerfEvent(PerfEventRingBuffer* ring_buffer,
                                                             const perf_event_header& header);

DmaFenceSignaledPerfEvent ConsumeDmaFenceSignaledPerfEvent(PerfEventRingBuffer* ring_buffer,
                                                           const perf_event_header& header);
}  // namespace orbit_linux_tracing

#endif  // LINUX_TRACING_PERF_EVENT_READERS_H_
