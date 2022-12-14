// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_SIGNALING_IQ_SENDER_H_
#define REMOTING_SIGNALING_IQ_SENDER_H_

#include <map>
#include <string>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "remoting/signaling/signal_strategy.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace buzz {
class XmlElement;
}  // namespace buzz

namespace remoting {

class IqRequest;
class SignalStrategy;

// IqSender handles sending iq requests and routing of responses to
// those requests.
class IqSender : public SignalStrategy::Listener {
 public:
  // Callback that is called when an Iq response is received. Called
  // with the |response| set to nullptr in case of a timeout.
  typedef base::Callback<void(IqRequest* request,
                              const buzz::XmlElement* response)> ReplyCallback;

  explicit IqSender(SignalStrategy* signal_strategy);
  ~IqSender() override;

  // Send an iq stanza. Returns an IqRequest object that represends
  // the request. |callback| is called when response to |stanza| is
  // received. Destroy the returned IqRequest to cancel the callback.
  // Caller must take ownership of the result. Result must be
  // destroyed before sender is destroyed.
  scoped_ptr<IqRequest> SendIq(scoped_ptr<buzz::XmlElement> stanza,
                               const ReplyCallback& callback);

  // Same as above, but also formats the message.
  scoped_ptr<IqRequest> SendIq(const std::string& type,
                               const std::string& addressee,
                               scoped_ptr<buzz::XmlElement> iq_body,
                               const ReplyCallback& callback);

  // SignalStrategy::Listener implementation.
  void OnSignalStrategyStateChange(SignalStrategy::State state) override;
  bool OnSignalStrategyIncomingStanza(const buzz::XmlElement* stanza) override;

 private:
  typedef std::map<std::string, IqRequest*> IqRequestMap;
  friend class IqRequest;

  // Helper function used to create iq stanzas.
  static scoped_ptr<buzz::XmlElement> MakeIqStanza(
      const std::string& type,
      const std::string& addressee,
      scoped_ptr<buzz::XmlElement> iq_body);

  // Removes |request| from the list of pending requests. Called by IqRequest.
  void RemoveRequest(IqRequest* request);

  SignalStrategy* signal_strategy_;
  IqRequestMap requests_;

  DISALLOW_COPY_AND_ASSIGN(IqSender);
};

// This call must only be used on the thread it was created on.
class IqRequest : public  base::SupportsWeakPtr<IqRequest> {
 public:
  IqRequest(IqSender* sender, const IqSender::ReplyCallback& callback,
            const std::string& addressee);
  ~IqRequest();

  // Sets timeout for the request. When the timeout expires the
  // callback is called with the |response| set to nullptr.
  void SetTimeout(base::TimeDelta timeout);

 private:
  friend class IqSender;

  void CallCallback(const buzz::XmlElement* stanza);
  void OnTimeout();

  // Called by IqSender when a response is received.
  void OnResponse(const buzz::XmlElement* stanza);

  void DeliverResponse(scoped_ptr<buzz::XmlElement> stanza);

  IqSender* sender_;
  IqSender::ReplyCallback callback_;
  std::string addressee_;

  DISALLOW_COPY_AND_ASSIGN(IqRequest);
};

}  // namespace remoting

#endif  // REMOTING_SIGNALING_IQ_SENDER_H_
