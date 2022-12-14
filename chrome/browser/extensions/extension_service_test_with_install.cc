// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_service_test_with_install.h"

#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/extensions/crx_installer.h"
#include "chrome/browser/extensions/extension_creator.h"
#include "chrome/browser/extensions/extension_error_reporter.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/notification_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {

struct ExtensionsOrder {
  bool operator()(const scoped_refptr<const Extension>& a,
                  const scoped_refptr<const Extension>& b) {
    return a->name() < b->name();
  }
};

// Helper method to set up a WindowedNotificationObserver to wait for a
// specific CrxInstaller to finish if we don't know the value of the
// |installer| yet.
bool IsCrxInstallerDone(extensions::CrxInstaller** installer,
                        const content::NotificationSource& source,
                        const content::NotificationDetails& details) {
  return content::Source<extensions::CrxInstaller>(source).ptr() == *installer;
}

}  // namespace

ExtensionServiceTestWithInstall::ExtensionServiceTestWithInstall()
    : installed_(nullptr),
      was_update_(false),
      unloaded_reason_(UnloadedExtensionInfo::REASON_UNDEFINED),
      expected_extensions_count_(0),
      override_external_install_prompt_(
          FeatureSwitch::prompt_for_external_extensions(), false),
      registry_observer_(this) {}

ExtensionServiceTestWithInstall::~ExtensionServiceTestWithInstall() {}

void ExtensionServiceTestWithInstall::InitializeExtensionService(
    const ExtensionServiceInitParams& params) {
  ExtensionServiceTestBase::InitializeExtensionService(params);

  registry_observer_.Add(registry());
}

// static
std::vector<base::string16> ExtensionServiceTestWithInstall::GetErrors() {
  const std::vector<base::string16>* errors =
      ExtensionErrorReporter::GetInstance()->GetErrors();
  std::vector<base::string16> ret_val;

  for (const base::string16& error : *errors) {
    std::string utf8_error = base::UTF16ToUTF8(error);
    if (utf8_error.find(".svn") == std::string::npos) {
      ret_val.push_back(error);
    }
  }

  // The tests rely on the errors being in a certain order, which can vary
  // depending on how filesystem iteration works.
  std::stable_sort(ret_val.begin(), ret_val.end());

  return ret_val;
}

void ExtensionServiceTestWithInstall::PackCRX(const base::FilePath& dir_path,
                                              const base::FilePath& pem_path,
                                              const base::FilePath& crx_path) {
  // Use the existing pem key, if provided.
  base::FilePath pem_output_path;
  if (pem_path.value().empty()) {
    pem_output_path = crx_path.DirName().AppendASCII("temp.pem");
  } else {
    ASSERT_TRUE(base::PathExists(pem_path));
  }

  ASSERT_TRUE(base::DeleteFile(crx_path, false));

  scoped_ptr<ExtensionCreator> creator(new ExtensionCreator());
  ASSERT_TRUE(creator->Run(dir_path,
                           crx_path,
                           pem_path,
                           pem_output_path,
                           ExtensionCreator::kOverwriteCRX));

  ASSERT_TRUE(base::PathExists(crx_path));
}

const Extension* ExtensionServiceTestWithInstall::PackAndInstallCRX(
    const base::FilePath& dir_path,
    const base::FilePath& pem_path,
    InstallState install_state,
    int creation_flags) {
  base::FilePath crx_path;
  base::ScopedTempDir temp_dir;
  EXPECT_TRUE(temp_dir.CreateUniqueTempDir());
  crx_path = temp_dir.path().AppendASCII("temp.crx");

  PackCRX(dir_path, pem_path, crx_path);
  return InstallCRX(crx_path, install_state, creation_flags);
}

const Extension* ExtensionServiceTestWithInstall::PackAndInstallCRX(
    const base::FilePath& dir_path,
    const base::FilePath& pem_path,
    InstallState install_state) {
  return PackAndInstallCRX(dir_path, pem_path, install_state,
                           Extension::NO_FLAGS);
}

const Extension* ExtensionServiceTestWithInstall::PackAndInstallCRX(
    const base::FilePath& dir_path,
    InstallState install_state) {
  return PackAndInstallCRX(dir_path, base::FilePath(), install_state,
                           Extension::NO_FLAGS);
}

// Attempts to install an extension. Use INSTALL_FAILED if the installation
// is expected to fail.
// If |install_state| is INSTALL_UPDATED, and |expected_old_name| is
// non-empty, expects that the existing extension's title was
// |expected_old_name|.
const Extension* ExtensionServiceTestWithInstall::InstallCRX(
    const base::FilePath& path,
    InstallState install_state,
    int creation_flags,
    const std::string& expected_old_name) {
  InstallCRXInternal(path, creation_flags);
  return VerifyCrxInstall(path, install_state, expected_old_name);
}

// Attempts to install an extension. Use INSTALL_FAILED if the installation
// is expected to fail.
const Extension* ExtensionServiceTestWithInstall::InstallCRX(
    const base::FilePath& path,
    InstallState install_state,
    int creation_flags) {
  return InstallCRX(path, install_state, creation_flags, std::string());
}

// Attempts to install an extension. Use INSTALL_FAILED if the installation
// is expected to fail.
const Extension* ExtensionServiceTestWithInstall::InstallCRX(
    const base::FilePath& path,
    InstallState install_state) {
  return InstallCRX(path, install_state, Extension::NO_FLAGS);
}

const Extension* ExtensionServiceTestWithInstall::InstallCRXFromWebStore(
    const base::FilePath& path,
    InstallState install_state) {
  InstallCRXInternal(path, Extension::FROM_WEBSTORE);
  return VerifyCrxInstall(path, install_state);
}

const Extension* ExtensionServiceTestWithInstall::InstallCRXWithLocation(
    const base::FilePath& crx_path,
    Manifest::Location install_location,
    InstallState install_state) {
  EXPECT_TRUE(base::PathExists(crx_path))
      << "Path does not exist: "<< crx_path.value().c_str();
  // no client (silent install)
  scoped_refptr<CrxInstaller> installer(
      CrxInstaller::CreateSilent(service()));
  installer->set_install_source(install_location);

  content::WindowedNotificationObserver observer(
      extensions::NOTIFICATION_CRX_INSTALLER_DONE,
      content::NotificationService::AllSources());
  installer->InstallCrx(crx_path);
  observer.Wait();

  return VerifyCrxInstall(crx_path, install_state);
}

const Extension* ExtensionServiceTestWithInstall::VerifyCrxInstall(
    const base::FilePath& path,
    InstallState install_state) {
  return VerifyCrxInstall(path, install_state, std::string());
}

const Extension* ExtensionServiceTestWithInstall::VerifyCrxInstall(
    const base::FilePath& path,
    InstallState install_state,
    const std::string& expected_old_name) {
  std::vector<base::string16> errors = GetErrors();
  const Extension* extension = nullptr;
  if (install_state != INSTALL_FAILED) {
    if (install_state == INSTALL_NEW)
      ++expected_extensions_count_;

    EXPECT_TRUE(installed_) << path.value();
    // If and only if INSTALL_UPDATED, it should have the is_update flag.
    EXPECT_EQ(install_state == INSTALL_UPDATED, was_update_)
        << path.value();
    // If INSTALL_UPDATED, old_name_ should match the given string.
    if (install_state == INSTALL_UPDATED && !expected_old_name.empty())
      EXPECT_EQ(expected_old_name, old_name_);
    EXPECT_EQ(0u, errors.size()) << path.value();

    if (install_state == INSTALL_WITHOUT_LOAD) {
      EXPECT_EQ(0u, loaded_.size()) << path.value();
    } else {
      EXPECT_EQ(1u, loaded_.size()) << path.value();
      size_t actual_extension_count =
          registry()->enabled_extensions().size() +
          registry()->disabled_extensions().size();
      EXPECT_EQ(expected_extensions_count_, actual_extension_count) <<
          path.value();
      extension = loaded_[0].get();
      EXPECT_TRUE(service()->GetExtensionById(extension->id(), false))
          << path.value();
    }

    for (std::vector<base::string16>::iterator err = errors.begin();
      err != errors.end(); ++err) {
      LOG(ERROR) << *err;
    }
  } else {
    EXPECT_FALSE(installed_) << path.value();
    EXPECT_EQ(0u, loaded_.size()) << path.value();
    EXPECT_EQ(1u, errors.size()) << path.value();
  }

  installed_ = nullptr;
  was_update_ = false;
  old_name_ = "";
  loaded_.clear();
  ExtensionErrorReporter::GetInstance()->ClearErrors();
  return extension;
}

void ExtensionServiceTestWithInstall::PackCRXAndUpdateExtension(
    const std::string& id,
    const base::FilePath& dir_path,
    const base::FilePath& pem_path,
    UpdateState expected_state) {
  base::ScopedTempDir temp_dir;
  EXPECT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath crx_path = temp_dir.path().AppendASCII("temp.crx");

  PackCRX(dir_path, pem_path, crx_path);
  UpdateExtension(id, crx_path, expected_state);
}

void ExtensionServiceTestWithInstall::UpdateExtension(
    const std::string& id,
    const base::FilePath& in_path,
    UpdateState expected_state) {
  ASSERT_TRUE(base::PathExists(in_path));

  // We need to copy this to a temporary location because Update() will delete
  // it.
  base::FilePath path = temp_dir().path();
  path = path.Append(in_path.BaseName());
  ASSERT_TRUE(base::CopyFile(in_path, path));

  int previous_enabled_extension_count =
      registry()->enabled_extensions().size();
  int previous_installed_extension_count =
      previous_enabled_extension_count +
      registry()->disabled_extensions().size();

  extensions::CrxInstaller* installer = nullptr;
  content::WindowedNotificationObserver observer(
      extensions::NOTIFICATION_CRX_INSTALLER_DONE,
      base::Bind(&IsCrxInstallerDone, &installer));
  service()->UpdateExtension(extensions::CRXFileInfo(id, path), true,
                             &installer);

  if (installer)
    observer.Wait();
  else
    base::RunLoop().RunUntilIdle();

  std::vector<base::string16> errors = GetErrors();
  int error_count = errors.size();
  int enabled_extension_count = registry()->enabled_extensions().size();
  int installed_extension_count =
      enabled_extension_count + registry()->disabled_extensions().size();

  int expected_error_count = (expected_state == FAILED) ? 1 : 0;
  EXPECT_EQ(expected_error_count, error_count) << path.value();

  if (expected_state <= FAILED) {
    EXPECT_EQ(previous_enabled_extension_count,
              enabled_extension_count);
    EXPECT_EQ(previous_installed_extension_count,
              installed_extension_count);
  } else {
    int expected_installed_extension_count =
        (expected_state >= INSTALLED) ? 1 : 0;
    int expected_enabled_extension_count =
        (expected_state >= ENABLED) ? 1 : 0;
    EXPECT_EQ(expected_installed_extension_count,
              installed_extension_count);
    EXPECT_EQ(expected_enabled_extension_count,
              enabled_extension_count);
  }

  // Update() should the temporary input file.
  EXPECT_FALSE(base::PathExists(path));
}

void ExtensionServiceTestWithInstall::UninstallExtension(const std::string& id,
                                                         bool use_helper) {
  UninstallExtension(id, use_helper, Extension::ENABLED);
}

void ExtensionServiceTestWithInstall::UninstallExtension(
    const std::string& id,
    bool use_helper,
    Extension::State expected_state) {
  // Verify that the extension is installed.
  base::FilePath extension_path = extensions_install_dir().AppendASCII(id);
  EXPECT_TRUE(base::PathExists(extension_path));
  size_t pref_key_count = GetPrefKeyCount();
  EXPECT_GT(pref_key_count, 0u);
  ValidateIntegerPref(id, "state", expected_state);

  // Uninstall it.
  if (use_helper) {
    EXPECT_TRUE(ExtensionService::UninstallExtensionHelper(
        service(), id, extensions::UNINSTALL_REASON_FOR_TESTING));
  } else {
    EXPECT_TRUE(service()->UninstallExtension(
        id, extensions::UNINSTALL_REASON_FOR_TESTING,
        base::Bind(&base::DoNothing), nullptr));
  }
  --expected_extensions_count_;

  // We should get an unload notification.
  EXPECT_FALSE(unloaded_id_.empty());
  EXPECT_EQ(id, unloaded_id_);

  // Verify uninstalled state.
  size_t new_pref_key_count = GetPrefKeyCount();
  if (new_pref_key_count == pref_key_count) {
    ValidateIntegerPref(id, "state",
                        Extension::EXTERNAL_EXTENSION_UNINSTALLED);
  } else {
    EXPECT_EQ(new_pref_key_count, pref_key_count - 1);
  }

  // The extension should not be in the service anymore.
  EXPECT_FALSE(service()->GetInstalledExtension(id));
  base::RunLoop().RunUntilIdle();

  // The directory should be gone.
  EXPECT_FALSE(base::PathExists(extension_path));
}

void ExtensionServiceTestWithInstall::TerminateExtension(
    const std::string& id) {
  const Extension* extension = service()->GetInstalledExtension(id);
  if (!extension) {
    ADD_FAILURE();
    return;
  }
  service()->TrackTerminatedExtensionForTest(extension);
}

void ExtensionServiceTestWithInstall::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const Extension* extension) {
  loaded_.push_back(make_scoped_refptr(extension));
  // The tests rely on the errors being in a certain order, which can vary
  // depending on how filesystem iteration works.
  std::stable_sort(loaded_.begin(), loaded_.end(), ExtensionsOrder());
}

void ExtensionServiceTestWithInstall::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const Extension* extension,
    UnloadedExtensionInfo::Reason reason) {
  unloaded_id_ = extension->id();
  unloaded_reason_ = reason;
  extensions::ExtensionList::iterator i =
      std::find(loaded_.begin(), loaded_.end(), extension);
      // TODO(erikkay) fix so this can be an assert.  Right now the tests
      // are manually calling clear() on loaded_, so this isn't doable.
      if (i == loaded_.end())
        return;
      loaded_.erase(i);
}

void ExtensionServiceTestWithInstall::OnExtensionWillBeInstalled(
    content::BrowserContext* browser_context,
    const Extension* extension,
    bool is_update,
    bool from_ephemeral,
    const std::string& old_name) {
  installed_ = extension;
  was_update_ = is_update;
  old_name_ = old_name;
}

// Create a CrxInstaller and install the CRX file.
// Instead of calling this method yourself, use InstallCRX(), which does extra
// error checking.
void ExtensionServiceTestWithInstall::InstallCRXInternal(
    const base::FilePath& crx_path,
    int creation_flags) {
  ASSERT_TRUE(base::PathExists(crx_path))
      << "Path does not exist: "<< crx_path.value().c_str();
  scoped_refptr<CrxInstaller> installer(
      CrxInstaller::CreateSilent(service()));
  installer->set_creation_flags(creation_flags);
  if (!(creation_flags & Extension::WAS_INSTALLED_BY_DEFAULT))
    installer->set_allow_silent_install(true);

  content::WindowedNotificationObserver observer(
      extensions::NOTIFICATION_CRX_INSTALLER_DONE,
      content::Source<extensions::CrxInstaller>(installer.get()));

  installer->InstallCrx(crx_path);

  observer.Wait();
}

}  // namespace extensions
