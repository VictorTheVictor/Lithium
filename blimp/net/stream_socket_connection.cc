// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "blimp/net/stream_socket_connection.h"

#include "blimp/net/stream_packet_reader.h"
#include "blimp/net/stream_packet_writer.h"

namespace blimp {

StreamSocketConnection::StreamSocketConnection(
    scoped_ptr<net::StreamSocket> socket)
    : BlimpConnection(make_scoped_ptr(new StreamPacketReader(socket.get())),
                      make_scoped_ptr(new StreamPacketWriter(socket.get()))),
      socket_(socket.Pass()) {
  DCHECK(socket_);
}

StreamSocketConnection::~StreamSocketConnection() {}

}  // namespace blimp
