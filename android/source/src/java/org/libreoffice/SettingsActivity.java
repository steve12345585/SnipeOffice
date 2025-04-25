/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
package org.libreoffice;

import android.os.Bundle;
import androidx.fragment.app.FragmentActivity;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceGroup;

public class SettingsActivity extends FragmentActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Display the fragment as the main content.
        getSupportFragmentManager().beginTransaction()
            .replace(android.R.id.content, new SettingsFragment())
            .commit();
    }

    public static class SettingsFragment extends PreferenceFragmentCompat {

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            setPreferencesFromResource(R.xml.libreoffice_preferences, rootKey);
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            if(!BuildConfig.ALLOW_EDITING) {
                PreferenceGroup generalGroup = findPreference("PREF_CATEGORY_GENERAL");
                generalGroup.removePreference(generalGroup.findPreference("ENABLE_EXPERIMENTAL"));
                generalGroup.removePreference(generalGroup.findPreference("ENABLE_DEVELOPER"));
            }
        }
    }
}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
