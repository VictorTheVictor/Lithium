// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_OMNIBOX_ALTERNATE_NAV_INFOBAR_DELEGATE_H_
#define CHROME_BROWSER_UI_OMNIBOX_ALTERNATE_NAV_INFOBAR_DELEGATE_H_

#include "base/memory/scoped_ptr.h"
#include "components/infobars/core/infobar_delegate.h"
#include "components/omnibox/browser/autocomplete_match.h"

class Profile;

namespace content {
class WebContents;
}

class AlternateNavInfoBarDelegate : public infobars::InfoBarDelegate {
 public:
  ~AlternateNavInfoBarDelegate() override;

  // Creates an alternate nav infobar and delegate and adds the infobar to the
  // infobar service for |web_contents|.
  static void Create(content::WebContents* web_contents,
                     const base::string16& text,
                     const AutocompleteMatch& match,
                     const GURL& search_url);

  base::string16 GetMessageTextWithOffset(size_t* link_offset) const;
  base::string16 GetLinkText() const;
  GURL GetLinkURL() const;
  bool LinkClicked(WindowOpenDisposition disposition);

 private:
  AlternateNavInfoBarDelegate(Profile* profile,
                              const base::string16& text,
                              const AutocompleteMatch& match,
                              const GURL& search_url);

  // Returns an alternate nav infobar that owns |delegate|.
  static scoped_ptr<infobars::InfoBar> CreateInfoBar(
      scoped_ptr<AlternateNavInfoBarDelegate> delegate);

  // InfoBarDelegate:
  Type GetInfoBarType() const override;
  int GetIconId() const override;
  gfx::VectorIconId GetVectorIconId() const override;

  Profile* profile_;
  const base::string16 text_;
  const AutocompleteMatch match_;
  const GURL search_url_;

  DISALLOW_COPY_AND_ASSIGN(AlternateNavInfoBarDelegate);
};

#endif  // CHROME_BROWSER_UI_OMNIBOX_ALTERNATE_NAV_INFOBAR_DELEGATE_H_
