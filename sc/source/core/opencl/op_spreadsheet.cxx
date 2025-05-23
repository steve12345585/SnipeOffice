/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "op_spreadsheet.hxx"

#include <rtl/math.hxx>
#include <formula/vectortoken.hxx>

#include <algorithm>
#include <sstream>

using namespace formula;

namespace sc::opencl {

void OpVLookup::GenSlidingWindowFunction(outputstream &ss,
            const std::string &sSymName, SubArguments &vSubArguments)
{
    GenerateFunctionDeclaration( sSymName, vSubArguments, ss );
    ss << "{\n";
    ss << "    int gid0=get_global_id(0);\n";
    ss << "    double tmp = CreateDoubleError(NOTAVAILABLE);\n";
    ss << "    double intermediate = DBL_MAX;\n";
    ss << "    int singleIndex = gid0;\n";
    ss << "    int rowNum = -1;\n";

    GenTmpVariables(ss,vSubArguments);
    int arg=0;
    CheckSubArgumentIsNan(ss,vSubArguments,arg++);
    int secondParaWidth = 1;

    // tdf#99512 - for now only allow non-dynamic indices (the
    // common-case) to validate consistent return types vs. the input.
    int index = 0;
    int indexArg = vSubArguments.size() - 2;
    if (vSubArguments[indexArg]->GetFormulaToken()->GetType() == formula::svDouble)
    {
        const formula::FormulaDoubleToken *dblToken = static_cast<const FormulaDoubleToken *>(vSubArguments[indexArg]->GetFormulaToken());
        index = ::rtl::math::approxFloor(dblToken->GetDouble());
    }

    if (vSubArguments[1]->GetFormulaToken()->GetType() != formula::svDoubleVectorRef)
        throw Unhandled(__FILE__, __LINE__); // unusual vlookup.

    FormulaToken *tmpCur = vSubArguments[1]->GetFormulaToken();
    const formula::DoubleVectorRefToken*pCurDVR = static_cast<const formula::DoubleVectorRefToken *>(tmpCur);
    const std::vector<VectorRefArray> items = pCurDVR->GetArrays();

    secondParaWidth = items.size();

    if (index < 1 || index > secondParaWidth)
        throw Unhandled(__FILE__, __LINE__); // oob index.

    if (items[index - 1].mpStringArray)
    {
        rtl_uString **pStrings = items[index - 1].mpStringArray;
        for (size_t i = 0; i < pCurDVR->GetArrayLength(); ++i)
        {
            if (pStrings[i] != nullptr)
            {   // TODO: the GroupTokenConverter should do better.
                throw Unhandled(__FILE__, __LINE__); // mixed arguments.
            }
        }
    }


    arg += secondParaWidth;
    CheckSubArgumentIsNan(ss,vSubArguments,arg++);

    if (vSubArguments.size() == static_cast<unsigned int>(3+(secondParaWidth-1)))
    {
        ss << "    double tmp";
        ss << 3+(secondParaWidth-1);
        ss << "= 1;\n";
    }
    else
    {
        CheckSubArgumentIsNan(ss,vSubArguments,arg++);
    }

    if (vSubArguments[1]->GetFormulaToken()->GetType() == formula::svDoubleVectorRef)
    {
        tmpCur = vSubArguments[1]->GetFormulaToken();
        pCurDVR = static_cast<const formula::DoubleVectorRefToken *>(tmpCur);
        size_t nCurWindowSize = std::min(pCurDVR->GetArrayLength(), pCurDVR->GetRefRowSize());
        const int unrollSize = 8;

        ss << "\n";
        ss << "    int loop = ";
        if (!pCurDVR->IsStartFixed() && pCurDVR->IsEndFixed())
        {
            ss << "("<<nCurWindowSize<<" - gid0)/";
            ss << unrollSize<<";\n";
        }
        else if (pCurDVR->IsStartFixed() && !pCurDVR->IsEndFixed())
        {
            ss << "("<<nCurWindowSize<<" + gid0)/";
            ss << unrollSize<<";\n";
        }
        else
        {
            ss << nCurWindowSize<<"/"<< unrollSize<<";\n";
        }

        ss << "    if(tmp";
        ss << 3+(secondParaWidth-1);
        ss << " == 0) /* unsorted vlookup */\n";
        ss << "    {\n";

        for( int sorted = 0; sorted < 2; ++sorted ) // sorted vs unsorted vlookup cases
        {
            if( sorted == 1 )
            {
                ss << "    }\n";
                ss << "    else\n";
                ss << "    { /* sorted vlookup */ \n";
            }

            ss << "        for ( int j = 0;j< loop; j++)\n";
            ss << "        {\n";
            ss << "            int i = ";
            if (!pCurDVR->IsStartFixed()&& pCurDVR->IsEndFixed())
            {
                ss << "gid0 + j * "<< unrollSize <<";\n";
            }
            else
            {
                ss << "j * "<< unrollSize <<";\n";
            }
            if (!pCurDVR->IsStartFixed() && !pCurDVR->IsEndFixed())
            {
                ss << "            int doubleIndex = i+gid0;\n";
            }
            else
            {
                ss << "            int doubleIndex = i;\n";
            }

            for (int j = 0;j < unrollSize; j++)
            {
                CheckSubArgumentIsNan(ss,vSubArguments,1);

                if( sorted == 1 )
                {
                    ss << "            if((tmp0 - tmp1)>=0 && intermediate > (tmp0 -tmp1))\n";
                    ss << "            {\n";
                    ss << "                rowNum = doubleIndex;\n";
                    ss << "                intermediate = tmp0 - tmp1;\n";
                    ss << "            }\n";
                    ss << "            i++;\n";
                    ss << "            doubleIndex++;\n";
                }
                else
                {
                    ss << "            if(tmp0 == tmp1)\n";
                    ss << "            {\n";
                    ss << "                rowNum = doubleIndex;\n";
                    ss << "                break;\n";
                    ss << "            }\n";
                    ss << "            i++;\n";
                    ss << "            doubleIndex++;\n";
                }
            }
            ss << "        }\n\n";
        }

        ss << "    }\n";
        ss << "    if(rowNum!=-1 && tmp";
        ss << 3 + (secondParaWidth - 1);
        ss << " == 0)\n";
        ss << "    {\n";
        for (int j = 0; j < secondParaWidth; j++)
        {
            ss << "        if(tmp";
            ss << 2+(secondParaWidth-1);
            ss << " == ";
            ss << j+1;
            ss << ")\n";
            ss << "            tmp = ";
            vSubArguments[1+j]->GenDeclRef(ss);
            ss << "[rowNum];\n";
        }
        ss << "        return tmp;\n";
        ss << "    }\n";

        ss << "    if(tmp";
        ss << 3+(secondParaWidth-1);
        ss << " == 0) /* unsorted vlookup */\n";
        ss << "    {\n";

        for( int sorted = 0; sorted < 2; ++sorted ) // sorted vs unsorted vlookup cases
        {
            if( sorted == 1 )
            {
                ss << "    }\n";
                ss << "    else\n";
                ss << "    { /* sorted vlookup */ \n";
            }

            ss << "        for (int i = ";
            if (!pCurDVR->IsStartFixed() && pCurDVR->IsEndFixed())
            {
                ss << "gid0 + loop *"<<unrollSize<<"; i < ";
                ss << nCurWindowSize <<"; i++)\n";
            }
            else if (pCurDVR->IsStartFixed() && !pCurDVR->IsEndFixed())
            {
                ss << "0 + loop *"<<unrollSize<<"; i < gid0+";
                ss << nCurWindowSize <<"; i++)\n";
            }
            else
            {
                ss << "0 + loop *"<<unrollSize<<"; i < ";
                ss << nCurWindowSize <<"; i++)\n";
            }
            ss << "        {\n";
            if (!pCurDVR->IsStartFixed() && !pCurDVR->IsEndFixed())
            {
               ss << "            int doubleIndex = i+gid0;\n";
            }
            else
            {
               ss << "            int doubleIndex = i;\n";
            }
            CheckSubArgumentIsNan(ss,vSubArguments,1);

            if( sorted == 1 )
            {
                ss << "            if((tmp0 - tmp1)>=0 && intermediate > (tmp0 -tmp1))\n";
                ss << "            {\n";
                ss << "                rowNum = doubleIndex;\n";
                ss << "                intermediate = tmp0 - tmp1;\n";
                ss << "            }\n";
            }
            else
            {
                ss << "            if(tmp0 == tmp1)\n";
                ss << "            {\n";
                ss << "                rowNum = doubleIndex;\n";
                ss << "                break;\n";
                ss << "            }\n";
            }
            ss << "        }\n\n";
        }

        ss << "    }\n";
        ss << "    if(rowNum!=-1)\n";
        ss << "    {\n";

        for (int j = 0; j < secondParaWidth; j++)
        {
            ss << "        if(tmp";
            ss << 2+(secondParaWidth-1);
            ss << " == ";
            ss << j+1;
            ss << ")\n";
            ss << "            tmp = ";
            vSubArguments[1+j]->GenDeclRef(ss);
            ss << "[rowNum];\n";
        }
        ss << "        return tmp;\n";
        ss << "    }\n";
    }
    else
    {
        CheckSubArgumentIsNan(ss,vSubArguments,1);
        ss << "    if(tmp3 == 1)\n";
        ss << "    {\n";
        ss << "        tmp = tmp1;\n";
        ss << "    }else\n";
        ss << "    {\n";
        ss << "        if(tmp0 == tmp1)\n";
        ss << "            tmp = tmp1;\n";
        ss << "    }\n";
    }
    ss << "    return tmp;\n";
    ss << "}";
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
