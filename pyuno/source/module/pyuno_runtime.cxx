/* -*- Mode: C++; eval:(c-set-style "bsd"); tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#include <sal/log.hxx>
#include <config_folders.h>

#include "pyuno_impl.hxx"

#include <o3tl/any.hxx>
#include <osl/diagnose.h>
#include <osl/thread.h>
#include <osl/module.h>
#include <osl/process.h>
#include <rtl/ustrbuf.hxx>
#include <rtl/bootstrap.hxx>
#include <rtl/ref.hxx>

#include <typelib/typedescription.hxx>

#include <com/sun/star/lang/WrappedTargetRuntimeException.hpp>
#include <com/sun/star/beans/XMaterialHolder.hpp>
#include <com/sun/star/beans/theIntrospection.hpp>
#include <com/sun/star/container/XHierarchicalNameAccess.hpp>
#include <com/sun/star/script/Converter.hpp>
#include <com/sun/star/script/InvocationAdapterFactory.hpp>
#include <com/sun/star/script/XInvocation2.hpp>
#include <com/sun/star/reflection/theCoreReflection.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <comphelper/sequence.hxx>
#include <comphelper/servicehelper.hxx>
#include <cppuhelper/exc_hlp.hxx>

#include <vector>

using com::sun::star::uno::Reference;
using com::sun::star::uno::XInterface;
using com::sun::star::uno::Any;
using com::sun::star::uno::TypeDescription;
using com::sun::star::uno::Sequence;
using com::sun::star::uno::Type;
using com::sun::star::uno::UNO_QUERY;
using com::sun::star::uno::Exception;
using com::sun::star::uno::RuntimeException;
using com::sun::star::uno::XComponentContext;
using com::sun::star::lang::WrappedTargetRuntimeException;
using com::sun::star::lang::XSingleServiceFactory;
using com::sun::star::lang::XUnoTunnel;
using com::sun::star::reflection::theCoreReflection;
using com::sun::star::reflection::InvocationTargetException;
using com::sun::star::script::Converter;
using com::sun::star::script::XTypeConverter;
using com::sun::star::script::XInvocation;
using com::sun::star::beans::XMaterialHolder;
using com::sun::star::beans::theIntrospection;

namespace pyuno
{

static PyTypeObject RuntimeImpl_Type =
{
    PyVarObject_HEAD_INIT (&PyType_Type, 0)
    "pyuno_runtime",
    sizeof (RuntimeImpl),
    0,
    RuntimeImpl::del,
#if PY_VERSION_HEX >= 0x03080000
    0, // Py_ssize_t tp_vectorcall_offset
#else
    nullptr, // printfunc tp_print
#endif
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    0,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    0,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    0,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr
    , 0
#if PY_VERSION_HEX >= 0x03040000
    , nullptr
#if PY_VERSION_HEX >= 0x03080000
    , nullptr // vectorcallfunc tp_vectorcall
#if PY_VERSION_HEX < 0x03090000
#if defined __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    , nullptr // tp_print
#if defined __clang__
#pragma clang diagnostic pop
#endif
#endif
#if PY_VERSION_HEX >= 0x030C00A1
    , 0 // tp_watched
#endif
#if PY_VERSION_HEX >= 0x030D00A4
    , 0 // tp_versions_used
#endif
#endif
#endif
};

/*----------------------------------------------------------------------
  Runtime implementation
 -----------------------------------------------------------------------*/
/// @throws css::uno::RuntimeException
static void getRuntimeImpl( PyRef & globalDict, PyRef &runtimeImpl )
{
    PyThreadState * state = PyThreadState_Get();
    if( ! state )
    {
        throw RuntimeException( u"python global interpreter must be held (thread must be attached)"_ustr );
    }

    PyObject* pModule = PyImport_AddModule("__main__");

    if (!pModule)
    {
        throw RuntimeException(u"can't import __main__ module"_ustr);
    }

    globalDict = PyRef( PyModule_GetDict(pModule));

    if( ! globalDict.is() ) // FATAL !
    {
        throw RuntimeException(u"can't find __main__ module"_ustr);
    }
    runtimeImpl = PyDict_GetItemString( globalDict.get() , "pyuno_runtime" );
}

/// @throws RuntimeException
static PyRef importUnoModule( )
{
    // import the uno module
    PyRef module( PyImport_ImportModule( "uno" ), SAL_NO_ACQUIRE, NOT_NULL );
    if( PyErr_Occurred() )
    {
        PyRef excType, excValue, excTraceback;
        PyErr_Fetch( reinterpret_cast<PyObject **>(&excType), reinterpret_cast<PyObject**>(&excValue), reinterpret_cast<PyObject**>(&excTraceback));
        // As of Python 2.7 this gives a rather non-useful "<traceback object at 0xADDRESS>",
        // but it is the best we can do in the absence of uno._uno_extract_printable_stacktrace
        // Who knows, a future Python might print something better.
        PyRef str( PyObject_Str( excTraceback.get() ), SAL_NO_ACQUIRE );

        OUStringBuffer buf;
        buf.append( "python object raised an unknown exception (" );
        PyRef valueRep( PyObject_Repr( excValue.get() ), SAL_NO_ACQUIRE );
        buf.appendAscii( PyUnicode_AsUTF8( valueRep.get())).append( ", traceback follows\n" );
        buf.appendAscii( PyUnicode_AsUTF8( str.get() ) );
        buf.append( ")" );
        throw RuntimeException( buf.makeStringAndClear() );
    }
    PyRef dict( PyModule_GetDict( module.get() ) );
    return dict;
}

static void readLoggingConfig( sal_Int32 *pLevel, FILE **ppFile )
{
    *pLevel = LogLevel::NONE;
    *ppFile = nullptr;
    OUString fileName;
    osl_getModuleURLFromFunctionAddress(
        reinterpret_cast< oslGenericFunction >(readLoggingConfig),
        &fileName.pData );
    fileName = fileName.copy( fileName.lastIndexOf( '/' )+1 );
#ifdef MACOSX
    fileName += "../" LIBO_ETC_FOLDER "/";
#endif
    fileName += SAL_CONFIGFILE("pyuno" );
    rtl::Bootstrap bootstrapHandle( fileName );

    OUString str;
    if( bootstrapHandle.getFrom( u"PYUNO_LOGLEVEL"_ustr, str ) )
    {
        if ( str == "NONE" )
            *pLevel = LogLevel::NONE;
        else if ( str == "CALL" )
            *pLevel = LogLevel::CALL;
        else if ( str == "ARGS" )
            *pLevel = LogLevel::ARGS;
        else
        {
            fprintf( stderr, "unknown loglevel %s\n",
                     OUStringToOString( str, RTL_TEXTENCODING_UTF8 ).getStr() );
        }
    }
    if( *pLevel <= LogLevel::NONE )
        return;

    *ppFile = stdout;
    if( !bootstrapHandle.getFrom( u"PYUNO_LOGTARGET"_ustr, str ) )
        return;

    if ( str == "stdout" )
        *ppFile = stdout;
    else if ( str == "stderr" )
        *ppFile = stderr;
    else
    {
        osl_getSystemPathFromFileURL( str.pData, &str.pData);
        OString o = OUStringToOString( str, osl_getThreadTextEncoding() );

        oslProcessInfo data;
        data.Size = sizeof( data );
        if (osl_getProcessInfo(
            nullptr , osl_Process_IDENTIFIER , &data ) == osl_Process_E_None)
        {
            o += ".";
            o += OString::number(data.Ident);
        }

        *ppFile = fopen( o.getStr() , "w" );
        if ( *ppFile )
        {
            // do not buffer (useful if e.g. analyzing a crash)
            setvbuf( *ppFile, nullptr, _IONBF, 0 );
        }
        else
        {
            fprintf( stderr, "couldn't create file %s\n",
                     OUStringToOString( str, RTL_TEXTENCODING_UTF8 ).getStr() );

        }
    }
}

/*-------------------------------------------------------------------
 RuntimeImpl implementations
 *-------------------------------------------------------------------*/
PyRef stRuntimeImpl::create( const Reference< XComponentContext > &ctx )
{
    RuntimeImpl *me = PyObject_New (RuntimeImpl, &RuntimeImpl_Type);
    if( ! me )
        throw RuntimeException( u"cannot instantiate pyuno::RuntimeImpl"_ustr );
    me->cargo = nullptr;
    // must use a different struct here, as the PyObject_New
    // makes C++ unusable
    RuntimeCargo *c = new RuntimeCargo;
    readLoggingConfig( &(c->logLevel) , &(c->logFile) );
    log( c, LogLevel::CALL, "Instantiating pyuno bridge" );

    c->valid = true;
    c->xContext = ctx;
    c->xInvocation = Reference< XSingleServiceFactory > (
        ctx->getServiceManager()->createInstanceWithContext(
            u"com.sun.star.script.Invocation"_ustr,
            ctx ),
        css::uno::UNO_QUERY_THROW );

    c->xTypeConverter = Converter::create(ctx);
    if( ! c->xTypeConverter.is() )
        throw RuntimeException( u"pyuno: couldn't instantiate typeconverter service"_ustr );

    c->xCoreReflection = theCoreReflection::get(ctx);

    c->xAdapterFactory = css::script::InvocationAdapterFactory::create(ctx);

    c->xIntrospection = theIntrospection::get(ctx);

    Any a = ctx->getValueByName(u"/singletons/com.sun.star.reflection.theTypeDescriptionManager"_ustr);
    a >>= c->xTdMgr;
    if( ! c->xTdMgr.is() )
        throw RuntimeException( u"pyuno: couldn't retrieve typedescriptionmanager"_ustr );

    me->cargo =c;
    return PyRef( reinterpret_cast< PyObject * > ( me ), SAL_NO_ACQUIRE );
}

void  stRuntimeImpl::del(PyObject* self)
{
    RuntimeImpl *me = reinterpret_cast< RuntimeImpl * > ( self );
    if( me->cargo->logFile )
        fclose( me->cargo->logFile );
    delete me->cargo;
    PyObject_Del (self);
}


void Runtime::initialize( const Reference< XComponentContext > & ctx )
{
    PyRef globalDict, runtime;
    getRuntimeImpl( globalDict , runtime );
    RuntimeImpl *impl = reinterpret_cast< RuntimeImpl * > (runtime.get());

    if( runtime.is() && impl->cargo->valid )
    {
        throw RuntimeException(u"pyuno runtime has already been initialized before"_ustr );
    }
    PyRef keep( RuntimeImpl::create( ctx ) );
    PyDict_SetItemString( globalDict.get(), "pyuno_runtime" , keep.get() );
    Py_XINCREF( keep.get() );
}


bool Runtime::isInitialized()
{
    PyRef globalDict, runtime;
    getRuntimeImpl( globalDict , runtime );
    RuntimeImpl *impl = reinterpret_cast< RuntimeImpl * > (runtime.get());
    return runtime.is() && impl->cargo->valid;
}

Runtime::Runtime()
    : impl( nullptr )
{
    PyRef globalDict, runtime;
    getRuntimeImpl( globalDict , runtime );
    if( ! runtime.is() )
    {
        throw RuntimeException(
            u"pyuno runtime is not initialized, "
            "(the pyuno.bootstrap needs to be called before using any uno classes)"_ustr );
    }
    impl = reinterpret_cast< RuntimeImpl * > (runtime.get());
    Py_XINCREF( runtime.get() );
}

Runtime::Runtime( const Runtime & r )
{
    impl = r.impl;
    Py_XINCREF( reinterpret_cast< PyObject * >(impl) );
}

Runtime::~Runtime()
{
    Py_XDECREF( reinterpret_cast< PyObject * >(impl) );
}

Runtime & Runtime::operator = ( const Runtime & r )
{
    PyRef temp( reinterpret_cast< PyObject * >(r.impl) );
    Py_XINCREF( temp.get() );
    Py_XDECREF( reinterpret_cast< PyObject * >(impl) );
    impl = r.impl;
    return *this;
}

PyRef Runtime::any2PyObject (const Any &a ) const
{
    if( ! impl->cargo->valid )
    {
        throw RuntimeException(u"pyuno runtime must be initialized before calling any2PyObject"_ustr );
    }

    switch (a.getValueTypeClass ())
    {
    case css::uno::TypeClass_VOID:
    {
        Py_INCREF (Py_None);
        return PyRef(Py_None);
    }
    case css::uno::TypeClass_CHAR:
    {
        sal_Unicode c = *o3tl::forceAccess<sal_Unicode>(a);
        return PyRef( PyUNO_char_new( c , *this ), SAL_NO_ACQUIRE );
    }
    case css::uno::TypeClass_BOOLEAN:
    {
        bool b;
        if ((a >>= b) && b)
            return Py_True;
        else
            return Py_False;
    }
    case css::uno::TypeClass_BYTE:
    case css::uno::TypeClass_SHORT:
    case css::uno::TypeClass_UNSIGNED_SHORT:
    case css::uno::TypeClass_LONG:
    {
        sal_Int32 l = 0;
        a >>= l;
        return PyRef( PyLong_FromLong (l), SAL_NO_ACQUIRE );
    }
    case css::uno::TypeClass_UNSIGNED_LONG:
    {
        sal_uInt32 l = 0;
        a >>= l;
        return PyRef( PyLong_FromUnsignedLong (l), SAL_NO_ACQUIRE );
    }
    case css::uno::TypeClass_HYPER:
    {
        sal_Int64 l = 0;
        a >>= l;
        return PyRef( PyLong_FromLongLong (l), SAL_NO_ACQUIRE);
    }
    case css::uno::TypeClass_UNSIGNED_HYPER:
    {
        sal_uInt64 l = 0;
        a >>= l;
        return PyRef( PyLong_FromUnsignedLongLong (l), SAL_NO_ACQUIRE);
    }
    case css::uno::TypeClass_FLOAT:
    {
        float f = 0.0;
        a >>= f;
        return PyRef(PyFloat_FromDouble (f), SAL_NO_ACQUIRE);
    }
    case css::uno::TypeClass_DOUBLE:
    {
        double d = 0.0;
        a >>= d;
        return PyRef( PyFloat_FromDouble (d), SAL_NO_ACQUIRE);
    }
    case css::uno::TypeClass_STRING:
    {
        OUString tmp_ostr;
        a >>= tmp_ostr;
        return ustring2PyUnicode( tmp_ostr );
    }
    case css::uno::TypeClass_TYPE:
    {
        Type t;
        a >>= t;
        OString o = OUStringToOString( t.getTypeName(), RTL_TEXTENCODING_ASCII_US );
        return PyRef(
            PyUNO_Type_new (
                o.getStr(),  t.getTypeClass(), *this),
            SAL_NO_ACQUIRE);
    }
    case css::uno::TypeClass_ANY:
    {
        //I don't think this can happen.
        Py_INCREF (Py_None);
        return Py_None;
    }
    case css::uno::TypeClass_ENUM:
    {
        sal_Int32 l = *static_cast<sal_Int32 const *>(a.getValue());
        TypeDescription desc( a.getValueType() );
        if( desc.is() )
        {
            desc.makeComplete();
            typelib_EnumTypeDescription *pEnumDesc =
                reinterpret_cast<typelib_EnumTypeDescription *>(desc.get());
            for( int i = 0 ; i < pEnumDesc->nEnumValues ; i ++ )
            {
                if( pEnumDesc->pEnumValues[i] == l )
                {
                    OString v = OUStringToOString( OUString::unacquired(&pEnumDesc->ppEnumNames[i]), RTL_TEXTENCODING_ASCII_US);
                    OString e = OUStringToOString( OUString::unacquired(&pEnumDesc->aBase.pTypeName), RTL_TEXTENCODING_ASCII_US);
                    return PyRef( PyUNO_Enum_new(e.getStr(),v.getStr(), *this ), SAL_NO_ACQUIRE );
                }
            }
        }
        throw RuntimeException( "Any carries enum " + a.getValueTypeName() +
                " with invalid value " + OUString::number(l) );
    }
    case css::uno::TypeClass_EXCEPTION:
    case css::uno::TypeClass_STRUCT:
    {
        PyRef excClass = getClass( a.getValueTypeName(), *this );
        PyRef value = PyUNOStruct_new( a, getImpl()->cargo->xInvocation );
        PyRef argsTuple( PyTuple_New( 1 ) , SAL_NO_ACQUIRE, NOT_NULL );
        PyTuple_SetItem( argsTuple.get() , 0 , value.getAcquired() );
        PyRef ret( PyObject_CallObject( excClass.get() , argsTuple.get() ), SAL_NO_ACQUIRE );
        if( ! ret.is() )
        {
            throw RuntimeException( "Couldn't instantiate python representation of structured UNO type " +
                        a.getValueTypeName() );
        }

        if( auto e = o3tl::tryAccess<css::uno::Exception>(a) )
        {
            // add the message in a standard python way !
            PyRef args( PyTuple_New( 1 ), SAL_NO_ACQUIRE, NOT_NULL );

            PyRef pymsg = ustring2PyString( e->Message );
            PyTuple_SetItem( args.get(), 0 , pymsg.getAcquired() );
            // the exception base functions want to have an "args" tuple,
            // which contains the message
            PyObject_SetAttrString( ret.get(), "args", args.get() );
        }
        return ret;
    }
    case css::uno::TypeClass_SEQUENCE:
    {
        Sequence<Any> s;

        Sequence< sal_Int8 > byteSequence;
        if( a >>= byteSequence )
        {
            // byte sequence is treated in a special way because of performance reasons
            // @since 0.9.2
            return PyRef( PyUNO_ByteSequence_new( byteSequence, *this ), SAL_NO_ACQUIRE );
        }
        else
        {
            Reference< XTypeConverter > tc = getImpl()->cargo->xTypeConverter;
            tc->convertTo (a, cppu::UnoType<decltype(s)>::get()) >>= s;
            PyRef tuple( PyTuple_New (s.getLength()), SAL_NO_ACQUIRE, NOT_NULL);
            int i=0;
            try
            {
                for ( i = 0; i < s.getLength (); i++)
                {
                    PyRef element = any2PyObject (tc->convertTo (s[i], s[i].getValueType() ));
                    OSL_ASSERT( element.is() );
                    PyTuple_SetItem( tuple.get(), i, element.getAcquired() );
                }
            }
            catch( css::uno::Exception & )
            {
                for( ; i < s.getLength() ; i ++ )
                {
                    Py_INCREF( Py_None );
                    PyTuple_SetItem( tuple.get(), i,  Py_None );
                }
                throw;
            }
            return tuple;
        }
    }
    case css::uno::TypeClass_INTERFACE:
    {
        Reference<XInterface> tmp_interface;
        a >>= tmp_interface;
        if (!tmp_interface.is ())
            return Py_None;

        return PyUNO_new( a, getImpl()->cargo->xInvocation );
    }
    default:
    {
        throw RuntimeException( "Unknown UNO type class " + OUString::number(static_cast<int>(a.getValueTypeClass())) );
    }
    }
}

static Sequence< Type > invokeGetTypes( const Runtime & r , PyObject * o )
{
    Sequence< Type > ret;

    PyRef method( PyObject_GetAttrString( o , "getTypes" ), SAL_NO_ACQUIRE );
    raiseInvocationTargetExceptionWhenNeeded( r );
    if( method.is() && PyCallable_Check( method.get() ) )
    {
        PyRef types( PyObject_CallObject( method.get(), nullptr ) , SAL_NO_ACQUIRE );
        raiseInvocationTargetExceptionWhenNeeded( r );
        if( types.is() && PyTuple_Check( types.get() ) )
        {
            int size = PyTuple_Size( types.get() );

            // add the XUnoTunnel interface  for uno object identity concept (hack)
            ret.realloc( size + 1 );
            auto pret = ret.getArray();
            for( int i = 0 ; i < size ; i ++ )
            {
                Any a = r.pyObject2Any(PyTuple_GetItem(types.get(),i));
                a >>= pret[i];
            }
            pret[size] = cppu::UnoType<css::lang::XUnoTunnel>::get();
        }
    }
    return ret;
}

static OUString
lcl_ExceptionMessage(PyObject *const o, OUString const*const pWrapped)
{
    OUStringBuffer buf;
    buf.append("Couldn't convert ");
    PyRef reprString( PyObject_Str(o), SAL_NO_ACQUIRE );
    buf.appendAscii( PyUnicode_AsUTF8(reprString.get()) );
    buf.append(" to a UNO type");
    if (pWrapped)
    {
        buf.append("; caught exception: ");
        buf.append(*pWrapped);
    }
    return buf.makeStringAndClear();
}

// For Python 2.7 - see https://bugs.python.org/issue24161
// Fills aSeq and returns true if pObj is a valid iterator
bool Runtime::pyIterUnpack( PyObject *const pObj, Any &a ) const
{
    if( !PyIter_Check( pObj ))
        return false;

    PyObject *pItem = PyIter_Next( pObj );
    if( !pItem )
    {
        if( PyErr_Occurred() )
        {
            PyErr_Clear();
            return false;
        }
        return true;
    }

    ::std::vector<Any> items;
    do
    {
        PyRef rItem( pItem, SAL_NO_ACQUIRE );
        items.push_back( pyObject2Any( rItem.get() ) );
        pItem = PyIter_Next( pObj );
    }
    while( pItem );
    a <<= comphelper::containerToSequence(items);
    return true;
}

Any Runtime::pyObject2Any(const PyRef & source, enum ConversionMode mode) const
{
    if (!impl || !impl->cargo->valid)
    {
        throw RuntimeException(u"pyuno runtime must be initialized before calling any2PyObject"_ustr );
    }

    Any a;
    PyObject *o = source.get();
    if( Py_None == o )
    {

    }
    else if (PyLong_Check (o))
    {
        // Convert the Python 3 booleans that are actually of type PyLong.
        if(o == Py_True)
        {
            a <<= true;
        }
        else if(o == Py_False)
        {
            a <<= false;
        }
        else
        {
        sal_Int64 l = static_cast<sal_Int64>(PyLong_AsLong (o));
        if( l < 128 && l >= -128 )
        {
            sal_Int8 b = static_cast<sal_Int8>(l);
            a <<= b;
        }
        else if( l <= 0x7fff && l >= -0x8000 )
        {
            sal_Int16 s = static_cast<sal_Int16>(l);
            a <<= s;
        }
        else if( l <= SAL_CONST_INT64(0x7fffffff) &&
                 l >= -SAL_CONST_INT64(0x80000000) )
        {
            sal_Int32 l32 = static_cast<sal_Int32>(l);
            a <<= l32;
        }
        else
        {
            a <<= l;
        }
        }
    }
    else if (PyFloat_Check (o))
    {
        double d = PyFloat_AsDouble (o);
        a <<= d;
    }
    else if (PyBytes_Check(o) || PyUnicode_Check(o))
    {
        a <<= pyString2ustring(o);
    }
    else if (PyTuple_Check (o))
    {
        Sequence<Any> s (PyTuple_Size (o));
        auto sRange = asNonConstRange(s);
        for (Py_ssize_t i = 0; i < PyTuple_Size (o); i++)
        {
            sRange[i] = pyObject2Any (PyTuple_GetItem (o, i), mode );
        }
        a <<= s;
    }
    else if (PyList_Check (o))
    {
        Py_ssize_t l = PyList_Size (o);
        Sequence<Any> s (l);
        auto sRange = asNonConstRange(s);
        for (Py_ssize_t i = 0; i < l; i++)
        {
            sRange[i] = pyObject2Any (PyList_GetItem (o, i), mode );
        }
        a <<= s;
    }
    else if (!pyIterUnpack (o, a))
    {
        Runtime runtime;
        // should be removed, in case ByteSequence gets derived from String
        if( PyObject_IsInstance( o, getByteSequenceClass( runtime ).get() ) )
        {
            PyRef str(PyObject_GetAttrString( o , "value" ),SAL_NO_ACQUIRE);
            Sequence< sal_Int8 > seq;
            if( PyBytes_Check( str.get() ) )
            {
                seq = Sequence<sal_Int8 > (
                    reinterpret_cast<sal_Int8*>(PyBytes_AsString(str.get())), PyBytes_Size(str.get()));
            }
            a <<= seq;
        }
        else
        if( PyObject_IsInstance( o, getTypeClass( runtime ).get() ) )
        {
            Type t = PyType2Type( o );
            a <<= t;
        }
        else if( PyObject_IsInstance( o, getEnumClass( runtime ).get() ) )
        {
            a = PyEnum2Enum( o );
        }
        else if( isInstanceOfStructOrException( o ) )
        {
            PyRef struc(PyObject_GetAttrString( o , "value" ),SAL_NO_ACQUIRE);
            PyUNO * obj = reinterpret_cast<PyUNO*>(struc.get());
            Reference< XMaterialHolder > holder( obj->members->xInvocation, UNO_QUERY );
            if( !holder.is( ) )
            {
                throw RuntimeException(
                    u"struct or exception wrapper does not support XMaterialHolder"_ustr );
            }

            a = holder->getMaterial();

        }
        else if( PyObject_IsInstance( o, getPyUnoClass().get() ) )
        {
            PyUNO* o_pi = reinterpret_cast<PyUNO*>(o);
            a = o_pi->members->wrappedObject;
        }
        else if( PyObject_IsInstance( o, getPyUnoStructClass().get() ) )
        {
            PyUNO* o_pi = reinterpret_cast<PyUNO*>(o);
            Reference<XMaterialHolder> my_mh (o_pi->members->xInvocation, css::uno::UNO_QUERY_THROW);
            a = my_mh->getMaterial();
        }
        else if( PyObject_IsInstance( o, getCharClass( runtime ).get() ) )
        {
            a <<= PyChar2Unicode( o );
        }
        else if( PyObject_IsInstance( o, getAnyClass( runtime ).get() ) )
        {
            if( ACCEPT_UNO_ANY != mode )
            {
                throw RuntimeException(
                    u"uno.Any instance not accepted during method call, "
                    "use uno.invoke instead"_ustr );
            }

            a = pyObject2Any( PyRef( PyObject_GetAttrString( o , "value" ), SAL_NO_ACQUIRE) );
            Type t;
            pyObject2Any( PyRef( PyObject_GetAttrString( o, "type" ), SAL_NO_ACQUIRE ) ) >>= t;

            try
            {
                a = getImpl()->cargo->xTypeConverter->convertTo( a, t );
            }
            catch( const css::uno::Exception & e )
            {
                css::uno::Any anyEx = cppu::getCaughtException();
                throw WrappedTargetRuntimeException(
                        e.Message, e.Context, anyEx);
            }

        }
        else
        {
            Reference< XInterface > mappedObject;
            Reference< XInvocation > adapterObject;

            // instance already mapped out to the world ?
            PyRef2Adapter::iterator ii = impl->cargo->mappedObjects.find( PyRef( o ) );
            if( ii != impl->cargo->mappedObjects.end() )
            {
                adapterObject = ii->second;
            }

            if( adapterObject.is() )
            {
                // object got already bridged !
                auto pAdapter = comphelper::getFromUnoTunnel<Adapter>(adapterObject);

                mappedObject = impl->cargo->xAdapterFactory->createAdapter(
                    adapterObject, pAdapter->getWrappedTypes() );
            }
            else
            {
                try {
                    Sequence<Type> interfaces = invokeGetTypes(*this, o);
                    if (interfaces.getLength())
                    {
                        rtl::Reference<Adapter> pAdapter = new Adapter( o, interfaces );
                        mappedObject =
                            getImpl()->cargo->xAdapterFactory->createAdapter(
                                pAdapter, interfaces );

                        // keep a list of exported objects to ensure object identity !
                        impl->cargo->mappedObjects[ PyRef(o) ] =
                            css::uno::WeakReference< XInvocation > ( pAdapter );
                    }
                } catch (InvocationTargetException const& e) {
                    OUString const msg(lcl_ExceptionMessage(o, &e.Message));
                    throw WrappedTargetRuntimeException( // re-wrap that
                            msg, e.Context, e.TargetException);
                }
            }
            if( mappedObject.is() )
            {
                a <<= mappedObject;
            }
            else
            {
                OUString const msg(lcl_ExceptionMessage(o, nullptr));
                throw RuntimeException(msg);
            }
        }
    }
    return a;
}

Any Runtime::extractUnoException( const PyRef & excType, const PyRef &excValue, const PyRef &excTraceback) const
{
    OUString str;
    Any ret;
    if( excTraceback.is() )
    {
        Exception e;
        PyRef unoModule;
        if ( impl )
        {
            try
            {
                unoModule = impl->cargo->getUnoModule();
            }
            catch (const Exception &ei)
            {
                e=ei;
            }
        }
        if( unoModule.is() )
        {
            PyRef extractTraceback(
                PyDict_GetItemString(unoModule.get(),"_uno_extract_printable_stacktrace" ) );

            if( PyCallable_Check(extractTraceback.get()) )
            {
                PyRef args( PyTuple_New( 1), SAL_NO_ACQUIRE, NOT_NULL );
                PyTuple_SetItem( args.get(), 0, excTraceback.getAcquired() );
                PyRef pyStr( PyObject_CallObject( extractTraceback.get(),args.get() ), SAL_NO_ACQUIRE);
                str = OUString::fromUtf8(PyUnicode_AsUTF8(pyStr.get()));
            }
            else
            {
                str = "Couldn't find uno._uno_extract_printable_stacktrace";
            }
        }
        else
        {
            str = "Could not load uno.py, no stacktrace available";
            if ( !e.Message.isEmpty() )
            {
                str += " (Error loading uno.py: " + e.Message + ")";
            }
        }

    }
    else
    {
        // it may occur, that no traceback is given (e.g. only native code below)
        str = "no traceback available";
    }

    if( isInstanceOfStructOrException( excValue.get() ) )
    {
        ret = pyObject2Any( excValue );
    }
    else
    {
        OUStringBuffer buf;
        PyRef typeName( PyObject_Str( excType.get() ), SAL_NO_ACQUIRE );
        if( typeName.is() )
        {
            buf.appendAscii( PyUnicode_AsUTF8( typeName.get() ) );
        }
        else
        {
            buf.append( "no typename available" );
        }
        buf.append( ": " );
        PyRef valueRep( PyObject_Str( excValue.get() ), SAL_NO_ACQUIRE );
        if( valueRep.is() )
        {
            buf.appendAscii( PyUnicode_AsUTF8( valueRep.get()));
        }
        else
        {
            buf.append( "Couldn't convert exception value to a string" );
        }
        buf.append( ", traceback follows\n" );
        if( !str.isEmpty() )
        {
            buf.append( str );
            buf.append( "\n" );
        }
        else
        {
            buf.append( ", no traceback available\n" );
        }
        RuntimeException e(buf.makeStringAndClear());
        SAL_WARN("pyuno.runtime", "Python exception: " << e.Message);
        ret <<= e;
    }
    return ret;
}


PyThreadAttach::PyThreadAttach( PyInterpreterState *interp)
    : m_isNewState(false)
{
    // note: *may* be called recursively, with PyThreadDetach between  - in
    // that case, don't create *new* PyThreadState but reuse!
    tstate = PyGILState_GetThisThreadState(); // from TLS, possibly detached
    if (!tstate)
    {
        m_isNewState = true;
        tstate = PyThreadState_New( interp );
    }
    if( !tstate  )
        throw RuntimeException( u"Couldn't create a pythreadstate"_ustr );
    PyEval_AcquireThread( tstate);
}

PyThreadAttach::~PyThreadAttach()
{
    if (m_isNewState)
    {   // Clear needs GIL!
        PyThreadState_Clear( tstate );
        // note: PyThreadState_Delete(tstate) cannot be called, it will assert
        // because it requires a PyThreadState to be set, but not the tstate!
        PyThreadState_DeleteCurrent();
    }
    else
    {
        PyEval_ReleaseThread( tstate );
    }
}

PyThreadDetach::PyThreadDetach()
{
    tstate = PyThreadState_Get();
    PyEval_ReleaseThread( tstate );
    // tstate must not be deleted here! lots of pointers to it on the stack
}

    /** Acquires the global interpreter lock again

    */
PyThreadDetach::~PyThreadDetach()
{
    PyEval_AcquireThread( tstate );
}


PyRef const & RuntimeCargo::getUnoModule()
{
    if( ! dictUnoModule.is() )
    {
        dictUnoModule = importUnoModule();
    }
    return dictUnoModule;
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
