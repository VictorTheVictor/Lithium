// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.signin;

import android.accounts.Account;
import android.content.Context;
import android.os.AsyncTask;

import org.chromium.base.Callback;
import org.chromium.base.Log;
import org.chromium.base.ObserverList;
import org.chromium.base.ThreadUtils;
import org.chromium.base.VisibleForTesting;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.sync.signin.AccountManagerHelper;

/**
* Android wrapper of AccountTrackerService which provides access from the java layer.
* It offers the capability of fetching and seeding system accounts into AccountTrackerService in C++
* layer, and notifies observers when it is complete.
*/
@JNINamespace("signin::android")
public class AccountTrackerService {
    private static final String TAG = "AccountService";
    private static AccountTrackerService sAccountTrackerService;

    private SystemAccountsSeedingStatus mSystemAccountsSeedingStatus;
    private boolean mSystemAccountsChanged;
    private boolean mSyncForceRefreshedForTest;

    private final Context mContext;

    private enum SystemAccountsSeedingStatus {
        SEEDING_NOT_STARTED,
        SEEDING_IN_PROGRESS,
        SEEDING_DONE
    }

    /**
    * Classes that want to listen for system accounts fetching and seeding should implement
    * this interface and register with {@link #addSystemAccountsSeededListener}.
    */
    public interface OnSystemAccountsSeededListener {
        // Called at the end of seedSystemAccounts().
        void onSystemAccountsSeedingComplete();
        // Called in invalidateAccountSeedStatus() indicating that accounts have changed.
        void onSystemAccountsChanged();
    }

    private final ObserverList<OnSystemAccountsSeededListener> mSystemAccountsSeedingObservers =
            new ObserverList<>();

    public static AccountTrackerService get(Context context) {
        ThreadUtils.assertOnUiThread();
        if (sAccountTrackerService == null) {
            sAccountTrackerService = new AccountTrackerService(context);
        }
        return sAccountTrackerService;
    }

    private AccountTrackerService(Context context) {
        mContext = context;
        mSystemAccountsSeedingStatus = SystemAccountsSeedingStatus.SEEDING_NOT_STARTED;
        mSystemAccountsChanged = false;
    }

    /**
    * Checks whether the account id <-> email mapping has been seeded into C++ layer.
    * If not, it automatically starts fetching the mapping and seeds it.
    * @return Whether the accounts have been seeded already.
    */
    public boolean checkAndSeedSystemAccounts() {
        ThreadUtils.assertOnUiThread();
        if (mSystemAccountsSeedingStatus == SystemAccountsSeedingStatus.SEEDING_DONE
                && !mSystemAccountsChanged) {
            return true;
        }
        if (mSystemAccountsSeedingStatus != SystemAccountsSeedingStatus.SEEDING_IN_PROGRESS) {
            seedSystemAccounts();
        }
        return false;
    }

    /**
    * Register an |observer| to observe system accounts seeding status.
    */
    public void addSystemAccountsSeededListener(OnSystemAccountsSeededListener observer) {
        ThreadUtils.assertOnUiThread();
        mSystemAccountsSeedingObservers.addObserver(observer);
        if (mSystemAccountsSeedingStatus == SystemAccountsSeedingStatus.SEEDING_DONE) {
            observer.onSystemAccountsSeedingComplete();
        }
    }

    private void seedSystemAccounts() {
        ThreadUtils.assertOnUiThread();
        mSystemAccountsChanged = false;
        mSyncForceRefreshedForTest = false;
        final AccountIdProvider accountIdProvider = AccountIdProvider.getInstance();
        if (accountIdProvider.canBeUsed(mContext, null)) {
            mSystemAccountsSeedingStatus = SystemAccountsSeedingStatus.SEEDING_IN_PROGRESS;
        } else {
            mSystemAccountsSeedingStatus = SystemAccountsSeedingStatus.SEEDING_NOT_STARTED;
            return;
        }
        AccountManagerHelper.get(mContext).getGoogleAccounts(new Callback<Account[]>() {
            @Override
            public void onResult(final Account[] accounts) {
                new AsyncTask<Void, Void, String[][]>() {
                    @Override
                    public String[][] doInBackground(Void... params) {
                        Log.d(TAG, "Getting id/email mapping");
                        String[][] accountIdNameMap = new String[2][accounts.length];
                        for (int i = 0; i < accounts.length; ++i) {
                            accountIdNameMap[0][i] =
                                    accountIdProvider.getAccountId(mContext, accounts[i].name);
                            accountIdNameMap[1][i] = accounts[i].name;
                        }
                        return accountIdNameMap;
                    }
                    @Override
                    public void onPostExecute(String[][] accountIdNameMap) {
                        if (mSyncForceRefreshedForTest) return;
                        if (mSystemAccountsChanged) {
                            seedSystemAccounts();
                            return;
                        }
                        if (areAccountIdsValid(accountIdNameMap[0])) {
                            nativeSeedAccountsInfo(accountIdNameMap[0], accountIdNameMap[1]);
                            mSystemAccountsSeedingStatus = SystemAccountsSeedingStatus.SEEDING_DONE;
                            notifyObserversOnSeedingComplete();
                        } else {
                            Log.w(TAG, "Invalid mapping of id/email");
                            seedSystemAccounts();
                        }
                    }
                }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
            }
        });
    }

    private boolean areAccountIdsValid(String[] accountIds) {
        for (int i = 0; i < accountIds.length; ++i) {
            if (accountIds[i] == null) return false;
        }
        return true;
    }

    private void notifyObserversOnSeedingComplete() {
        for (OnSystemAccountsSeededListener observer : mSystemAccountsSeedingObservers) {
            observer.onSystemAccountsSeedingComplete();
        }
    }

    /**
    * Seed system accounts into AccountTrackerService synchronously for test purpose.
    */
    @VisibleForTesting
    public void syncForceRefreshForTest(String[] accountIds, String[] accountNames) {
        ThreadUtils.assertOnUiThread();
        mSystemAccountsSeedingStatus = SystemAccountsSeedingStatus.SEEDING_IN_PROGRESS;
        mSyncForceRefreshedForTest = true;
        nativeSeedAccountsInfo(accountIds, accountNames);
        mSystemAccountsSeedingStatus = SystemAccountsSeedingStatus.SEEDING_DONE;
    }

    /**
    * Notifies the AccountTrackerService about changed system accounts. without actually triggering
    * @param reSeedAccounts Whether to also start seeding the new account information immediately.
    */
    public void invalidateAccountSeedStatus(boolean reSeedAccounts) {
        ThreadUtils.assertOnUiThread();
        mSystemAccountsChanged = true;
        notifyObserversOnAccountsChange();
        if (reSeedAccounts) checkAndSeedSystemAccounts();
    }

    private void notifyObserversOnAccountsChange() {
        for (OnSystemAccountsSeededListener observer : mSystemAccountsSeedingObservers) {
            observer.onSystemAccountsChanged();
        }
    }

    private static native void nativeSeedAccountsInfo(String[] gaiaIds, String[] accountNames);
}
