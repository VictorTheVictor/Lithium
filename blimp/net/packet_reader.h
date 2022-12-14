// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BLIMP_NET_PACKET_READER_H_
#define BLIMP_NET_PACKET_READER_H_

#include "blimp/net/blimp_net_export.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"

namespace blimp {

// Interface to describe a reader which can read variable-length data packets
// from a connection.
class BLIMP_NET_EXPORT PacketReader {
 public:
  virtual ~PacketReader() {}

  // Reads a packet from the connection.
  // Returns the size of the packet, in bytes, if the read operation executed
  // successfully.
  // Returns ERR_IO_PENDING if the operation will be executed asynchronously.
  //     |cb| is later invoked with the packet size or an error code.
  // All other return values indicate errors.
  virtual int ReadPacket(const scoped_refptr<net::GrowableIOBuffer>& buf,
                         const net::CompletionCallback& cb) = 0;
};

}  // namespace blimp

#endif  // BLIMP_NET_PACKET_READER_H_
