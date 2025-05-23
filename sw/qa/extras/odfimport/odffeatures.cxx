/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <swmodeltestbase.hxx>

#include <config_features.h>

namespace
{
class Test : public SwModelTestBase
{
public:
    Test()
        : SwModelTestBase("/sw/qa/extras/odfimport/data/", "writer8")
    {
    }
};

CPPUNIT_TEST_FIXTURE(Test, testFeatureText) { createSwDoc("feature_text.odt"); }

CPPUNIT_TEST_FIXTURE(Test, testFeatureTextBold) { createSwDoc("feature_text_bold.odt"); }

CPPUNIT_TEST_FIXTURE(Test, testFeatureTextItalic) { createSwDoc("feature_text_italic.odt"); }

} // end of anonymous namespace
CPPUNIT_PLUGIN_IMPLEMENT();
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
