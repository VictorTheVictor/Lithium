// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SESSIONS_CORE_PERSISTENT_TAB_RESTORE_SERVICE_H_
#define COMPONENTS_SESSIONS_CORE_PERSISTENT_TAB_RESTORE_SERVICE_H_

#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "components/sessions/core/sessions_export.h"
#include "components/sessions/core/tab_restore_service.h"
#include "components/sessions/core/tab_restore_service_client.h"
#include "components/sessions/core/tab_restore_service_helper.h"

class PersistentTabRestoreServiceTest;

namespace sessions {

// Tab restore service that persists data on disk.
class SESSIONS_EXPORT PersistentTabRestoreService : public TabRestoreService {
 public:
  // Does not take ownership of |time_factory|.
  PersistentTabRestoreService(scoped_ptr<TabRestoreServiceClient> client,
                              TimeFactory* time_factory);

  ~PersistentTabRestoreService() override;

  // TabRestoreService:
  void AddObserver(TabRestoreServiceObserver* observer) override;
  void RemoveObserver(TabRestoreServiceObserver* observer) override;
  void CreateHistoricalTab(LiveTab* live_tab, int index) override;
  void BrowserClosing(LiveTabContext* context) override;
  void BrowserClosed(LiveTabContext* context) override;
  void ClearEntries() override;
  const Entries& entries() const override;
  std::vector<LiveTab*> RestoreMostRecentEntry(LiveTabContext* context,
                                               int host_desktop_type) override;
  Tab* RemoveTabEntryById(SessionID::id_type id) override;
  std::vector<LiveTab*> RestoreEntryById(
      LiveTabContext* context,
      SessionID::id_type id,
      int host_desktop_type,
      WindowOpenDisposition disposition) override;
  void LoadTabsFromLastSession() override;
  bool IsLoaded() const override;
  void DeleteLastSession() override;
  void Shutdown() override;

 private:
  friend class ::PersistentTabRestoreServiceTest;

  class Delegate;

  // Exposed for testing.
  Entries* mutable_entries();
  void PruneEntries();

  scoped_ptr<TabRestoreServiceClient> client_;
  scoped_ptr<Delegate> delegate_;
  TabRestoreServiceHelper helper_;

  DISALLOW_COPY_AND_ASSIGN(PersistentTabRestoreService);
};

}  // namespace sessions

#endif  // COMPONENTS_SESSIONS_CORE_PERSISTENT_TAB_RESTORE_SERVICE_H_
