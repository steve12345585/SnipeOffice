/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#ifdef _WIN32
#if !defined WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <systools/win32/test_desktop.hxx>
#endif
#if defined(_WIN32) && defined(_DEBUG)
#include <dbghelp.h>
#include <sal/backtrace.hxx>
#include <signal.h>
#endif

#ifdef UNX
#include <sys/resource.h>
#endif

#include <cstdlib>
#include <iostream>
#include <string>
#include <sal/log.hxx>
#include <sal/types.h>
#include <cppunittester/protectorfactory.hxx>
#include <osl/module.h>
#include <osl/module.hxx>
#include <osl/process.h>
#include <osl/thread.h>
#include <rtl/character.hxx>
#include <rtl/string.hxx>
#include <rtl/strbuf.hxx>
#include <rtl/ustring.hxx>
#include <sal/main.h>

#include <cppunit/CompilerOutputter.h>
#include <cppunit/Exception.h>
#include <cppunit/TestFailure.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/plugin/PlugInManager.h>
#include <cppunit/plugin/DynamicLibraryManagerException.h>
#include <cppunit/portability/Stream.h>

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <string_view>
#include <utility>

namespace {

void usageFailure() {
    std::cerr
        << ("Usage: cppunittester (--protector <shared-library-path>"
            " <function-symbol>)* <shared-library-path>")
        << std::endl;
    std::exit(EXIT_FAILURE);
}

OUString getArgument(sal_Int32 index) {
    OUString arg;
    osl_getCommandArg(index, &arg.pData);
    return arg;
}

std::string convertLazy(std::u16string_view s16) {
    OString s8(OUStringToOString(s16, osl_getThreadTextEncoding()));
    static_assert(sizeof (sal_Int32) <= sizeof (std::string::size_type), "must be at least the same size");
        // ensure following cast is legitimate
    return std::string(s8);
}

//Output how long each test took
class TimingListener
    : public CppUnit::TestListener
{
public:
    TimingListener()
        : m_nStartTime(0)
    {
    }
    TimingListener(const TimingListener&) = delete;
    TimingListener& operator=(const TimingListener&) = delete;

    void startTest( CppUnit::Test *test) override
    {
        std::cout << "[_RUN_____] " << test->getName() << std::endl;
        m_nStartTime = osl_getGlobalTimer();
    }

    void endTest( CppUnit::Test *test ) override
    {
        sal_uInt32 nEndTime = osl_getGlobalTimer();
        std::cout << test->getName() << " finished in: "
            << nEndTime-m_nStartTime << "ms" << std::endl;
    }

private:
    sal_uInt32 m_nStartTime;
};

// Setup an env variable so that temp file (or other) can
// have a useful value to identify the source
class EyecatcherListener
    : public CppUnit::TestListener
{
public:
    EyecatcherListener() = default;
    EyecatcherListener(const EyecatcherListener&) = delete;
    EyecatcherListener& operator=(const EyecatcherListener&) = delete;
    void startTest( CppUnit::Test* test) override
    {
        rtl::OStringBuffer tn(test->getName());
        for(int i = 0; i < tn.getLength(); i++)
        {
            if(!rtl::isAsciiAlphanumeric(static_cast<unsigned char>(tn[i])))
            {
                tn[i] = '_';
            }
        }
        tn.append('_');
#ifdef WIN32
        _putenv_s("LO_TESTNAME", tn.getStr());
#else
        setenv("LO_TESTNAME", tn.getStr(), true);
#endif
    }

    void endTest( CppUnit::Test* /* test */ ) override
    {
    }
};

class LogFailuresAsTheyHappen : public CppUnit::TestListener
{
public:
    virtual void addFailure( const CppUnit::TestFailure &failure ) override
    {
        printFailureLocation( failure.sourceLine() );
        printFailedTestName( failure );
        printFailureMessage( failure );
    }

private:
    static void printFailureLocation( const CppUnit::SourceLine &sourceLine )
    {
        if ( !sourceLine.isValid() )
            std::cerr << "unknown:0:";
        else
            std::cerr << sourceLine.fileName() << ":" << sourceLine.lineNumber() << ":";
    }

    static void printFailedTestName( const CppUnit::TestFailure &failure )
    {
        std::cerr << failure.failedTestName() << std::endl;
    }

    static void printFailureMessage( const CppUnit::TestFailure &failure )
    {
        std::cerr << failure.thrownException()->message().shortDescription() << std::endl;
        std::cerr << failure.thrownException()->message().details() << std::endl;
    }
};

struct test_name_compare
{
    explicit test_name_compare(std::string aName):
        maName(std::move(aName))
    {
    }

    bool operator()(const std::string& rCmp)
    {
        size_t nPos = maName.find(rCmp);
        if (nPos == std::string::npos)
            return false;

        size_t nEndPos = nPos + rCmp.size();
        return nEndPos == maName.size();
    }

    std::string maName;
};

bool addRecursiveTests(const std::vector<std::string>& test_names, CppUnit::Test* pTest, CppUnit::TestRunner& rRunner)
{
    bool ret(false);
    for (int i = 0; i < pTest->getChildTestCount(); ++i)
    {
        CppUnit::Test* pNewTest = pTest->getChildTestAt(i);
        ret |= addRecursiveTests(test_names, pNewTest, rRunner);
        if (std::any_of(test_names.begin(), test_names.end(), test_name_compare(pNewTest->getName())))
        {
            rRunner.addTest(pNewTest);
            ret = true;
        }
    }
    return ret;
}

//Allow the whole uniting testing framework to be run inside a "Protector"
//which knows about uno exceptions, so it can print the content of the
//exception before falling over and dying
class CPPUNIT_API ProtectedFixtureFunctor
    : public CppUnit::Functor
{
private:
    const std::string &testlib;
    const std::string &args;
    std::vector<CppUnit::Protector *> &protectors;
    CppUnit::TestResult &result;
public:
    ProtectedFixtureFunctor(const std::string& testlib_, const std::string &args_, std::vector<CppUnit::Protector*> &protectors_, CppUnit::TestResult &result_)
        : testlib(testlib_)
        , args(args_)
        , protectors(protectors_)
        , result(result_)
    {
    }
    ProtectedFixtureFunctor(const ProtectedFixtureFunctor&) = delete;
    ProtectedFixtureFunctor& operator=(const ProtectedFixtureFunctor&) = delete;
    bool run() const
    {
#ifdef DISABLE_DYNLOADING

        // NOTE: Running cppunit unit tests on iOS was something I did
        // only very early (several years ago) when starting porting
        // this stuff to iOS. The complicated mechanisms to do build
        // such unit test single executables have surely largely
        // bit-rotted or been semi-intentionally broken since. This
        // stuff here left for information only. --tml 2014.

        // For iOS cppunit plugins aren't really "plugins" (shared
        // libraries), but just static archives. In the real main
        // program of a cppunit app, which calls the lo_main() that
        // the SAL_IMPLEMENT_MAIN() below expands to, we specifically
        // call the initialize methods of the CppUnitTestPlugIns that
        // we statically link to the app executable.
#else
        // The PlugInManager instance is deliberately leaked, so that the dynamic libraries it loads
        // are never unloaded (which could make e.g. pointers from other libraries' static data
        // structures to const data in those libraries, like some static OUString cache pointing at
        // a const OUStringLiteral, become dangling by the time those static data structures are
        // destroyed during exit):
        auto manager = new CppUnit::PlugInManager;
        try {
            manager->load(testlib, args);
        } catch (const CppUnit::DynamicLibraryManagerException &e) {
            std::cerr << "DynamicLibraryManagerException: \"" << e.what() << "\"\n";
            const char *pPath = getenv ("PATH");
            const size_t nPathLen = pPath ? strlen(pPath) : 0;
#ifdef _WIN32
            if (nPathLen > 256)
            {
                std::cerr << "Windows has significant build problems with long PATH variables ";
                std::cerr << "please check your PATH variable and re-autogen.\n";
            }
#endif
            std::cerr << "Path (length: " << nPathLen << ") is '" << pPath << "'\n";
            return false;
        }
#endif

        for (size_t i = 0; i < protectors.size(); ++i)
            result.pushProtector(protectors[i]);

        bool success;
        {
            CppUnit::TestResultCollector collector;
            result.addListener(&collector);

            LogFailuresAsTheyHappen logger;
            result.addListener(&logger);

            TimingListener timer;
            result.addListener(&timer);

            EyecatcherListener eye;
            result.addListener(&eye);

            // set this to track down files created before first test method
            std::string lib = testlib.substr(testlib.rfind('/')+1);
#ifdef WIN32
            _putenv_s("LO_TESTNAME", lib.c_str());
#else
            setenv("LO_TESTNAME", lib.c_str(), true);
#endif
            const char* pVal = getenv("CPPUNIT_TEST_NAME");

            CppUnit::TestRunner runner;
            if (pVal)
            {
                std::vector<std::string> test_names;
                boost::split(test_names, pVal, boost::is_any_of("\t "));
                CppUnit::Test* pTest = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
                bool const added(addRecursiveTests(test_names, pTest, runner));
                if (!added)
                {
                    std::cerr << "\nFatal error: CPPUNIT_TEST_NAME contains no valid tests\n";
                    // coverity[leaked_storage] - `manager` leaked on purpose
                    return false;
                }
            }
            else
            {
                runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
            }
            runner.run(result);

            CppUnit::CompilerOutputter outputter(&collector, CppUnit::stdCErr());
            outputter.setNoWrap();
            outputter.write();
            success = collector.wasSuccessful();
        }

        for (size_t i = 0; i < protectors.size(); ++i)
            result.popProtector();

        return success;
    }
    virtual bool operator()() const override
    {
        return run();
    }
};

#ifdef UNX

double get_time(timeval* time)
{
    double nTime = static_cast<double>(time->tv_sec);
    nTime += static_cast<double>(time->tv_usec)/1000000.0;
    return nTime;
}

OString generateTestName(std::u16string_view rPath)
{
    size_t nPathSep = rPath.rfind('/');
    size_t nAfterPathSep = (nPathSep != std::string_view::npos) ? (nPathSep + 1) : 0;
    std::u16string_view aTestName = rPath.substr(nAfterPathSep);
    return OUStringToOString(aTestName, RTL_TEXTENCODING_UTF8);
}

void reportResourceUsage(std::u16string_view rPath)
{
    OUString aFullPath = OUString::Concat(rPath) + ".resource.log";
    rusage resource_usage;
    getrusage(RUSAGE_SELF, &resource_usage);

    OString aPath = OUStringToOString(aFullPath, RTL_TEXTENCODING_UTF8);
    std::ofstream resource_file(aPath.getStr());
    resource_file << "Name = " << generateTestName(rPath) << std::endl;
    double nUserSpace = get_time(&resource_usage.ru_utime);
    double nKernelSpace = get_time(&resource_usage.ru_stime);
    resource_file << "UserSpace = " << nUserSpace << std::endl;
    resource_file << "KernelSpace = " << nKernelSpace << std::endl;
}
#else
void reportResourceUsage([[maybe_unused]] const OUString& /*rPath*/)
{
}
#endif

}

static bool main2()
{
    bool ok = false;
    OUString path;

#ifdef _WIN32
    //Disable Dr-Watson in order to crash simply without popup dialogs under
    //windows
    DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
    SetErrorMode(SEM_NOGPFAULTERRORBOX|dwMode);
#ifdef _DEBUG // These functions are present only in the debugging runtime
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG|_CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG|_CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG|_CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
    // Create a desktop, to avoid popups interfering with active user session,
    // because on Windows, we don't use svp vcl plugin for unit testing
    sal::systools::maybeCreateTestDesktop();
#endif

    std::vector<CppUnit::Protector *> protectors;
    CppUnit::TestResult result;
    std::string args;
    std::string testlib;
    sal_uInt32 index = 0;
    while (index < osl_getCommandArgCount())
    {
        OUString arg = getArgument(index);
        if (arg.startsWith("-env:CPPUNITTESTTARGET=", &path))
        {
            ++index;
            continue;
        }
        if (arg.startsWith("-env:"))
        {
            ++index;
            continue; // ignore it here - will be read later
        }
        if ( arg != "--protector" )
        {
            if (testlib.empty())
            {
                testlib = OUStringToOString(arg, osl_getThreadTextEncoding()).getStr();
                args += testlib;
            }
            else
            {
                args += ' ';
                args += OUStringToOString(arg, osl_getThreadTextEncoding()).getStr();
            }
            ++index;
            continue;
        }
        if (osl_getCommandArgCount() - index < 3) {
            usageFailure();
        }
        OUString lib(getArgument(index + 1));
        OUString sym(getArgument(index + 2));
#ifndef DISABLE_DYNLOADING
        osl::Module mod(lib, SAL_LOADMODULE_GLOBAL);
        oslGenericFunction fn = mod.getFunctionSymbol(sym);
        mod.release();
#else
        oslGenericFunction fn = 0;
        if (sym == "unoexceptionprotector")
            fn = (oslGenericFunction) unoexceptionprotector;
        else if (sym == "unobootstrapprotector")
            fn = (oslGenericFunction) unobootstrapprotector;
        else if (sym == "vclbootstrapprotector")
            fn = (oslGenericFunction) vclbootstrapprotector;
        else
        {
            std::cerr
                << "Only unoexceptionprotector or unobootstrapprotector protectors allowed"
                << std::endl;
            std::exit(EXIT_FAILURE);
        }
#endif
        if (fn == nullptr) {
            std::cerr
                << "Failure instantiating protector \"" << convertLazy(lib)
                << "\", \"" << convertLazy(sym) << '"' << std::endl;
            std::exit(EXIT_FAILURE);
        }
        CppUnit::Protector *protector =
            (*reinterpret_cast< cppunittester::ProtectorFactory * >(fn))();
        if (protector != nullptr) {
            protectors.push_back(protector);
        }
        index+=3;
    }

    ProtectedFixtureFunctor tests(testlib, args, protectors, result);
    ok = tests.run();

    reportResourceUsage(path);

    return ok;
}

#if defined(_WIN32) && defined(_DEBUG)

//Prints stack trace based on exception context record
static void printStack( PCONTEXT ctx )
{
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    STACKFRAME64        stack {};
    stack.AddrPC.Mode      = AddrModeFlat;
    stack.AddrStack.Mode   = AddrModeFlat;
    stack.AddrFrame.Mode   = AddrModeFlat;
#ifdef _M_AMD64
    stack.AddrPC.Offset    = ctx->Rip;
    stack.AddrStack.Offset = ctx->Rsp;
    stack.AddrFrame.Offset = ctx->Rsp;
#elif defined _M_ARM64
    stack.AddrPC.Offset    = ctx->Pc;
    stack.AddrStack.Offset = ctx->Sp;
    stack.AddrFrame.Offset = ctx->Fp;
#else
    stack.AddrPC.Offset    = ctx->Eip;
    stack.AddrStack.Offset = ctx->Esp;
    stack.AddrFrame.Offset = ctx->Ebp;
#endif

    DWORD symOptions = SymGetOptions();
    symOptions |= SYMOPT_LOAD_LINES;
    symOptions |= SYMOPT_FAIL_CRITICAL_ERRORS;
    symOptions = SymSetOptions(symOptions);

    SymInitialize( process, nullptr, TRUE ); //load symbols

    IMAGEHLP_LINE64 line{};
    line.SizeOfStruct = sizeof(line);

    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO pSymbol = reinterpret_cast<PSYMBOL_INFO>(buffer);

    for (;;)
    {
        //get next call from stack
        bool result = StackWalk64
        (
#ifdef _M_AMD64
            IMAGE_FILE_MACHINE_AMD64,
#elif defined _M_ARM64
            IMAGE_FILE_MACHINE_ARM64,
#else
            IMAGE_FILE_MACHINE_I386,
#endif
            process,
            thread,
            &stack,
            ctx,
            nullptr,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            nullptr
        );

        if( !result )
            break;

        //get symbol name for address
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME + 1;
        if (SymFromAddr(process, stack.AddrPC.Offset, nullptr, pSymbol))
            printf("\tat %s", pSymbol->Name);
        else
            printf("\tat unknown (Error in SymFromAddr=%#08lx)", GetLastError());

        DWORD disp;
        //try to get line
        if (SymGetLineFromAddr64(process, stack.AddrPC.Offset, &disp, &line))
        {
            printf(" in %s: line: %lu:\n", line.FileName, line.LineNumber);
        }
        else
        {
            //failed to get line
            printf(", address 0x%0I64X", stack.AddrPC.Offset);
            HMODULE hModule = nullptr;
            GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCTSTR>(stack.AddrPC.Offset), &hModule);

            char sModule[256];
            //at least print module name
            if (hModule != nullptr)
                GetModuleFileNameA(hModule, sModule, std::size(sModule));

            printf (" in %s\n", sModule);
        }
    }
}

// The exception filter function:
static LONG WINAPI ExpFilter(EXCEPTION_POINTERS* ex)
{
    // we only want this active on the Jenkins tinderboxes
    printf("*** Exception 0x%lx occurred ***\n\n",ex->ExceptionRecord->ExceptionCode);
    printStack(ex->ContextRecord);
    return EXCEPTION_EXECUTE_HANDLER;
}

static void AbortSignalHandler(int signal)
{
    if (signal == SIGABRT) {
        std::unique_ptr<sal::BacktraceState> bs = sal::backtrace_get(50);
        SAL_WARN("sal.cppunittester", "CAUGHT SIGABRT:\n" << sal::backtrace_to_string(bs.get()));
    }
}

SAL_IMPLEMENT_MAIN()
{
    // catch the kind of signal that is thrown when an assert fails, and log a stacktrace
    signal(SIGABRT, AbortSignalHandler);

    bool ok = false;
    // This magic kind of Windows-specific exception handling has to be in its own function
    // because it cannot be in a function that has objects with destructors.
    __try
    {
        ok = main2();
    }
    __except (ExpFilter(GetExceptionInformation()))
    {
    }
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

#else

SAL_IMPLEMENT_MAIN()
{
    bool ok = false;
    try
    {
        ok = main2();
    }
    catch (const std::exception& e)
    {
        SAL_WARN("vcl.app", "Fatal exception: " << e.what());
    }
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
