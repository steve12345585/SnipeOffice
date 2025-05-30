# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t; fill-column: 100 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CompilerTest_CompilerTest,compilerplugins_clang))

$(eval $(call gb_CompilerTest_add_exception_objects,compilerplugins_clang, \
    compilerplugins/clang/test/badstatics \
    compilerplugins/clang/test/blockblock \
    compilerplugins/clang/test/bufferadd \
    compilerplugins/clang/test/buriedassign \
    compilerplugins/clang/test/casttovoid \
    compilerplugins/clang/test/classmemaccess \
    compilerplugins/clang/test/collapseif \
    compilerplugins/clang/test/commaoperator \
    compilerplugins/clang/test/conditionalstring \
    compilerplugins/clang/test/constexprliteral \
    compilerplugins/clang/test/constmethod \
    compilerplugins/clang/test/constmove \
    compilerplugins/clang/test/consttobool \
    compilerplugins/clang/test/cow_wrapper \
    compilerplugins/clang/test/cppunitassertequals \
    compilerplugins/clang/test/crosscast \
    compilerplugins/clang/test/cstylecast \
    compilerplugins/clang/test/datamembershadow \
    compilerplugins/clang/test/dbgunhandledexception \
    compilerplugins/clang/test/elidestringvar \
    compilerplugins/clang/test/emptyif \
    compilerplugins/clang/test/expressionalwayszero \
    compilerplugins/clang/test/external \
    compilerplugins/clang/test/faileddyncast \
    compilerplugins/clang/test/fakebool \
    compilerplugins/clang/test/fieldcast \
    compilerplugins/clang/test/flatten \
    compilerplugins/clang/test/fragiledestructor \
    compilerplugins/clang/test/getstr \
    compilerplugins/clang/test/implicitboolconversion \
    compilerplugins/clang/test/implinheritancehelper \
    compilerplugins/clang/test/indentation \
    compilerplugins/clang/test/intvsfloat \
    compilerplugins/clang/test/locking2 \
    compilerplugins/clang/test/logexceptionnicely \
    compilerplugins/clang/test/loopvartoosmall \
    compilerplugins/clang/test/mapindex \
    compilerplugins/clang/test/makeshared \
    compilerplugins/clang/test/moveit \
    compilerplugins/clang/test/moveparam \
    compilerplugins/clang/test/mustoverride \
    compilerplugins/clang/test/noexcept \
    compilerplugins/clang/test/noexceptmove \
    compilerplugins/clang/test/nullptr \
    compilerplugins/clang/test/optionalbool \
    compilerplugins/clang/test/oslendian-1 \
    compilerplugins/clang/test/oslendian-2 \
    compilerplugins/clang/test/oslendian-3 \
    compilerplugins/clang/test/ostr \
    compilerplugins/clang/test/overridevirtual \
    compilerplugins/clang/test/passparamsbyref \
    compilerplugins/clang/test/passstuffbyref \
    compilerplugins/clang/test/pointerbool \
    compilerplugins/clang/test/rangedforcopy \
    compilerplugins/clang/test/reducevarscope \
    compilerplugins/clang/test/redundantcast \
    compilerplugins/clang/test/redundantfcast \
    compilerplugins/clang/test/redundantinline \
    compilerplugins/clang/test/redundantpointerops \
    compilerplugins/clang/test/redundantpreprocessor \
    compilerplugins/clang/test/refcounting \
    compilerplugins/clang/test/referencecasting \
    compilerplugins/clang/test/reftotemp \
    compilerplugins/clang/test/returnconstval \
    compilerplugins/clang/test/salcall \
    compilerplugins/clang/test/sallogareas \
    compilerplugins/clang/test/salunicodeliteral \
    compilerplugins/clang/test/selfinit \
    compilerplugins/clang/test/simplifyconstruct \
    compilerplugins/clang/test/simplifydynamiccast \
    compilerplugins/clang/test/simplifypointertobool \
    compilerplugins/clang/test/singlevalfields \
    compilerplugins/clang/test/staticconstexpr \
    compilerplugins/clang/test/staticconstfield \
    compilerplugins/clang/test/staticdynamic \
    compilerplugins/clang/test/staticmethods \
    compilerplugins/clang/test/stdfunction \
    compilerplugins/clang/test/stringadd \
    compilerplugins/clang/test/stringconcatauto \
    compilerplugins/clang/test/stringconcatliterals \
    compilerplugins/clang/test/stringconstant \
    compilerplugins/clang/test/stringliteralvar \
    compilerplugins/clang/test/stringstatic \
    compilerplugins/clang/test/stringview \
    compilerplugins/clang/test/stringviewdangle \
    compilerplugins/clang/test/stringviewparam \
    compilerplugins/clang/test/stringviewvar \
    compilerplugins/clang/test/trivialconstructor \
    compilerplugins/clang/test/trivialdestructor \
    compilerplugins/clang/test/typedefparam \
    compilerplugins/clang/test/typeidcomparison \
    compilerplugins/clang/test/unnecessarycatchthrow \
    compilerplugins/clang/test/unnecessarygetstr \
    compilerplugins/clang/test/unnecessaryoverride \
    compilerplugins/clang/test/unnecessaryoverride-dtor \
    compilerplugins/clang/test/unnecessaryparen \
    compilerplugins/clang/test/unnecessarylocking \
    compilerplugins/clang/test/unoaggregation \
    compilerplugins/clang/test/unoany \
    compilerplugins/clang/test/unoquery \
    compilerplugins/clang/test/unreffun \
    compilerplugins/clang/test/unsignedcompare \
    compilerplugins/clang/test/unusedcapturedefault \
    compilerplugins/clang/test/unusedenumconstants \
    compilerplugins/clang/test/unusedfields \
    compilerplugins/clang/test/unusedmember \
    compilerplugins/clang/test/unusedvariablecheck \
    compilerplugins/clang/test/unusedvariablemore \
    compilerplugins/clang/test/unusedvarsglobal \
    compilerplugins/clang/test/useuniqueptr \
    compilerplugins/clang/test/vclwidgets \
    compilerplugins/clang/test/weakbase \
    compilerplugins/clang/test/writeonlyvars \
    compilerplugins/clang/test/xmlimport \
))

$(eval $(call gb_CompilerTest_use_externals,compilerplugins_clang, \
    boost_headers \
    cppunit \
))

$(eval $(call gb_CompilerTest_use_udk_api,compilerplugins_clang))

# vim: set noet sw=4 ts=4:
