/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>
#include "helper/qahelper.hxx"
#include <document.hxx>
#include <docfunc.hxx>
#include <table.hxx>
#include <SolverSettings.hxx>

using namespace sc;

class SolverTest : public ScModelTestBase
{
public:
    SolverTest()
        : ScModelTestBase(u"sc/qa/unit/data"_ustr)
    {
    }

    std::vector<ModelConstraint> CreateConstraintsModelA();
    void TestConstraintsModelA(SolverSettings* pSettings);
};

// Creates a simple set of constraints for testing
std::vector<ModelConstraint> SolverTest::CreateConstraintsModelA()
{
    std::vector<ModelConstraint> aConstraints;

    ModelConstraint aConstr1;
    aConstr1.aLeftStr = "$C$1:$C$10";
    aConstr1.nOperator = CO_LESS_EQUAL;
    aConstr1.aRightStr = "100";
    aConstraints.push_back(aConstr1);

    ModelConstraint aConstr2;
    aConstr2.aLeftStr = "$F$5";
    aConstr2.nOperator = CO_EQUAL;
    aConstr2.aRightStr = "500";
    aConstraints.push_back(aConstr2);

    ModelConstraint aConstr3;
    aConstr3.aLeftStr = "$D$1:$D$5";
    aConstr3.nOperator = CO_BINARY;
    aConstr3.aRightStr = "";
    aConstraints.push_back(aConstr3);

    return aConstraints;
}

// Tests the contents of the three constraints
void SolverTest::TestConstraintsModelA(SolverSettings* pSettings)
{
    std::vector<ModelConstraint> aConstraints = pSettings->GetConstraints();

    CPPUNIT_ASSERT_EQUAL(u"$C$1:$C$10"_ustr, aConstraints[0].aLeftStr);
    CPPUNIT_ASSERT_EQUAL(CO_LESS_EQUAL, aConstraints[0].nOperator);
    CPPUNIT_ASSERT_EQUAL(u"100"_ustr, aConstraints[0].aRightStr);

    CPPUNIT_ASSERT_EQUAL(u"$F$5"_ustr, aConstraints[1].aLeftStr);
    CPPUNIT_ASSERT_EQUAL(CO_EQUAL, aConstraints[1].nOperator);
    CPPUNIT_ASSERT_EQUAL(u"500"_ustr, aConstraints[1].aRightStr);

    CPPUNIT_ASSERT_EQUAL(u"$D$1:$D$5"_ustr, aConstraints[2].aLeftStr);
    CPPUNIT_ASSERT_EQUAL(CO_BINARY, aConstraints[2].nOperator);
    CPPUNIT_ASSERT_EQUAL(u""_ustr, aConstraints[2].aRightStr);
}

/* This test creates a model in a single tab and test if the model info
 * is correctly stored in the object
 */
CPPUNIT_TEST_FIXTURE(SolverTest, testSingleModel)
{
    createScDoc();
    ScDocument* pDoc = getScDoc();
    ScTable* pTable = pDoc->FetchTable(0);
    std::shared_ptr<sc::SolverSettings> pSettings = pTable->GetSolverSettings();
    CPPUNIT_ASSERT(pSettings);

    // Test solver default settings on an empty tab
    // Here we only test default settings that are not engine-dependent
    CPPUNIT_ASSERT_EQUAL(u""_ustr, pSettings->GetParameter(SP_OBJ_CELL));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(OT_MAXIMIZE),
                         pSettings->GetParameter(SP_OBJ_TYPE).toInt32());
    CPPUNIT_ASSERT_EQUAL(u""_ustr, pSettings->GetParameter(SP_OBJ_VAL));
    CPPUNIT_ASSERT_EQUAL(u""_ustr, pSettings->GetParameter(SP_VAR_CELLS));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), pSettings->GetParameter(SP_CONSTR_COUNT).toInt32());

    // Create a simple model
    pSettings->SetParameter(SP_OBJ_CELL, u"$A$1"_ustr);
    pSettings->SetParameter(SP_OBJ_TYPE, OUString::number(OT_MINIMIZE));
    pSettings->SetParameter(SP_OBJ_VAL, OUString::number(0));
    pSettings->SetParameter(SP_VAR_CELLS, u"$D$1:$D$5"_ustr);
    std::vector<ModelConstraint> aConstraints = CreateConstraintsModelA();
    pSettings->SetConstraints(aConstraints);

    // Test if the model parameters were set
    CPPUNIT_ASSERT_EQUAL(u"$A$1"_ustr, pSettings->GetParameter(SP_OBJ_CELL));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(OT_MINIMIZE),
                         pSettings->GetParameter(SP_OBJ_TYPE).toInt32());
    CPPUNIT_ASSERT_EQUAL(u"0"_ustr, pSettings->GetParameter(SP_OBJ_VAL));
    CPPUNIT_ASSERT_EQUAL(u"$D$1:$D$5"_ustr, pSettings->GetParameter(SP_VAR_CELLS));

    // Test if the constraints were correctly set before saving
    CPPUNIT_ASSERT_EQUAL(sal_Int32(3), pSettings->GetParameter(SP_CONSTR_COUNT).toInt32());
    TestConstraintsModelA(pSettings.get());

    // Save and reload the file
    pSettings->SaveSolverSettings();
    saveAndReload(u"calc8"_ustr);
    pDoc = getScDoc();
    pTable = pDoc->FetchTable(0);
    pSettings = pTable->GetSolverSettings();
    CPPUNIT_ASSERT(pSettings);

    // Test if the model parameters remain set in the file
    CPPUNIT_ASSERT_EQUAL(u"$A$1"_ustr, pSettings->GetParameter(SP_OBJ_CELL));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(OT_MINIMIZE),
                         pSettings->GetParameter(SP_OBJ_TYPE).toInt32());
    CPPUNIT_ASSERT_EQUAL(u"0"_ustr, pSettings->GetParameter(SP_OBJ_VAL));
    CPPUNIT_ASSERT_EQUAL(u"$D$1:$D$5"_ustr, pSettings->GetParameter(SP_VAR_CELLS));

    // Test if the constraints remain correct after saving
    CPPUNIT_ASSERT_EQUAL(sal_Int32(3), pSettings->GetParameter(SP_CONSTR_COUNT).toInt32());
    TestConstraintsModelA(pSettings.get());
}

// Tests if references remain valid after a sheet is renamed
CPPUNIT_TEST_FIXTURE(SolverTest, tdf156815)
{
    createScDoc("ods/tdf156815.ods");
    ScDocument* pDoc = getScDoc();
    ScTable* pTable = pDoc->FetchTable(0);
    std::shared_ptr<sc::SolverSettings> pSettings = pTable->GetSolverSettings();
    CPPUNIT_ASSERT(pSettings);

    // Check current values in the solver model
    CPPUNIT_ASSERT_EQUAL(u"$Sheet2.$A$1"_ustr, pSettings->GetParameter(SP_OBJ_CELL));
    CPPUNIT_ASSERT_EQUAL(u"$Sheet2.$A$3:$B$3"_ustr, pSettings->GetParameter(SP_VAR_CELLS));

    std::vector<ModelConstraint> aConstraints = pSettings->GetConstraints();
    CPPUNIT_ASSERT_EQUAL(u"$Sheet2.$A$2"_ustr, aConstraints[0].aLeftStr);
    CPPUNIT_ASSERT_EQUAL(u"$Sheet2.$B$2"_ustr, aConstraints[0].aRightStr);

    // Rename Sheet2 to NewName
    ScDocFunc& rDocFunc = getScDocShell()->GetDocFunc();
    rDocFunc.RenameTable(1, u"NewName"_ustr, false, true);

    // Check whether the ranges where updated
    pSettings = pTable->GetSolverSettings();
    CPPUNIT_ASSERT(pSettings);
    CPPUNIT_ASSERT_EQUAL(u"$NewName.$A$1"_ustr, pSettings->GetParameter(SP_OBJ_CELL));
    CPPUNIT_ASSERT_EQUAL(u"$NewName.$A$3:$B$3"_ustr, pSettings->GetParameter(SP_VAR_CELLS));

    aConstraints = pSettings->GetConstraints();
    CPPUNIT_ASSERT_EQUAL(u"$NewName.$A$2"_ustr, aConstraints[0].aLeftStr);
    CPPUNIT_ASSERT_EQUAL(u"$NewName.$B$2"_ustr, aConstraints[0].aRightStr);
}

// Tests if settings for the DEPS and SCO solvers are kept in the file
CPPUNIT_TEST_FIXTURE(SolverTest, tdf158735)
{
    createScDoc("ods/tdf158735.ods");
    ScDocument* pDoc = getScDoc();

    // Test the non-default values of the DEPS model
    ScTable* pTable = pDoc->FetchTable(0);
    std::shared_ptr<sc::SolverSettings> pSettings = pTable->GetSolverSettings();
    CPPUNIT_ASSERT(pSettings);
    CPPUNIT_ASSERT_EQUAL(u"com.sun.star.comp.Calc.NLPSolver.DEPSSolverImpl"_ustr,
                         pSettings->GetParameter(SP_LO_ENGINE));
    CPPUNIT_ASSERT_EQUAL(u"0.45"_ustr, pSettings->GetParameter(SP_AGENT_SWITCH_RATE));
    CPPUNIT_ASSERT_EQUAL(u"0.85"_ustr, pSettings->GetParameter(SP_CROSSOVER_PROB));
    CPPUNIT_ASSERT_EQUAL(u"1500"_ustr, pSettings->GetParameter(SP_LEARNING_CYCLES));
    CPPUNIT_ASSERT_EQUAL(u"0"_ustr, pSettings->GetParameter(SP_ENHANCED_STATUS));

    // Test the non-default values of the SCO model
    pTable = pDoc->FetchTable(1);
    pSettings = pTable->GetSolverSettings();
    CPPUNIT_ASSERT(pSettings);
    CPPUNIT_ASSERT_EQUAL(u"com.sun.star.comp.Calc.NLPSolver.SCOSolverImpl"_ustr,
                         pSettings->GetParameter(SP_LO_ENGINE));
    CPPUNIT_ASSERT_EQUAL(u"180"_ustr, pSettings->GetParameter(SP_LIBRARY_SIZE));
    CPPUNIT_ASSERT_EQUAL(u"0.00055"_ustr, pSettings->GetParameter(SP_STAGNATION_TOLERANCE));
    CPPUNIT_ASSERT_EQUAL(u"1"_ustr, pSettings->GetParameter(SP_RND_STARTING_POINT));
    CPPUNIT_ASSERT_EQUAL(u"80"_ustr, pSettings->GetParameter(SP_STAGNATION_LIMIT));
}

// Tests if range addresses from XLSX files that belong to the same sheet are not imported
// with the sheet name, since it is unnecessary and clutters the dialog
CPPUNIT_TEST_FIXTURE(SolverTest, tdf156814)
{
    createScDoc("xlsx/tdf156814.xlsx");
    ScDocument* pDoc = getScDoc();

    ScTable* pTable = pDoc->FetchTable(0);
    std::shared_ptr<sc::SolverSettings> pSettings = pTable->GetSolverSettings();
    CPPUNIT_ASSERT(pSettings);
    // Ranges must not contain the sheet name
    CPPUNIT_ASSERT_EQUAL(u"$F$2"_ustr, pSettings->GetParameter(SP_OBJ_CELL));
    CPPUNIT_ASSERT_EQUAL(u"$A$2:$A$11,$C$2:$D$11"_ustr, pSettings->GetParameter(SP_VAR_CELLS));

    // Check also the constraints (ranges must not contain sheet name either)
    std::vector<ModelConstraint> aConstraints = pSettings->GetConstraints();
    CPPUNIT_ASSERT_EQUAL(u"$H$2:$H$11"_ustr, aConstraints[0].aLeftStr);
    CPPUNIT_ASSERT_EQUAL(u"$I$2:$I$11"_ustr, aConstraints[0].aRightStr);
    CPPUNIT_ASSERT_EQUAL(u"$H$2:$H$11"_ustr, aConstraints[1].aLeftStr);
    CPPUNIT_ASSERT_EQUAL(u"10"_ustr, aConstraints[1].aRightStr);
    CPPUNIT_ASSERT_EQUAL(u"$I$2:$I$11"_ustr, aConstraints[2].aLeftStr);
    CPPUNIT_ASSERT_EQUAL(u"0"_ustr, aConstraints[2].aRightStr);
}

// Tests if all named ranges and expressions are hidden in the existing model
CPPUNIT_TEST_FIXTURE(SolverTest, tdf160064)
{
    createScDoc("ods/SolverModel.ods");
    ScDocument* pDoc = getScDoc();

    ScTable* pTable = pDoc->FetchTable(0);
    ScRangeName* pNamedRanges = pTable->GetRangeName();
    ScRangeName::const_iterator it = pNamedRanges->begin();

    // There are 34 hidden named ranges and expressions in the file
    CPPUNIT_ASSERT_EQUAL(size_t(34), pNamedRanges->size());

    // All named ranges and expressions are hidden in the file
    while (it != pNamedRanges->end())
    {
        OUString sName = it->first;
        ScRangeData* pRangeData
            = pNamedRanges->findByUpperName(ScGlobal::getCharClass().uppercase(sName));
        CPPUNIT_ASSERT(pRangeData->HasType(ScRangeData::Type::Hidden));
        it++;
    }
}

CPPUNIT_PLUGIN_IMPLEMENT();
