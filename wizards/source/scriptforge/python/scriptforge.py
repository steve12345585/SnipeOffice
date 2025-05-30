# -*- coding: utf-8 -*-

#     Copyright 2020-2024 Jean-Pierre LEDURE, Rafael LIMA, @AmourSpirit, Alain ROMEDENNE

# =====================================================================================================================
# ===           The ScriptForge library and its associated libraries are Part of the SnipeOffice project.           ===
# ===                   Full documentation is available on https://help.SnipeOffice.org/                            ===
# =====================================================================================================================

# ScriptForge is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# ScriptForge is free software; you can redistribute it and/or modify it under the terms of either (at your option):

# 1) The Mozilla Public License, v. 2.0. If a copy of the MPL was not
# distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0/ .

# 2) The GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version. If a copy of the LGPL was not
# distributed with this file, see http://www.gnu.org/licenses/ .

"""
    ScriptForge libraries are an extensible and robust collection of macro scripting resources for SnipeOffice
    to be invoked from user Basic or Python macros. Users familiar with other BASIC macro variants often face hard
    times to dig into the extensive LibreOffice Application Programming Interface even for the simplest operations.
    By collecting most-demanded document operations in a set of easy to use, easy to read routines, users can now
    program document macros with much less hassle and get quicker results.

    The use of the ScriptForge interfaces in user scripts hides the complexity of the usual UNO interfaces.
    However, it does not replace them. At the opposite their coexistence is ensured.
    Indeed, ScriptForge provides a number of shortcuts to key UNO objects.

    The scriptforge.py module
        - describes the interfaces (classes and attributes) to be used in Python user scripts
          to run the services implemented in the standard modules shipped with LibreOffice
        - implements a protocol between those interfaces and, when appropriate, the corresponding ScriptForge
          Basic libraries implementing the requested services.

    The scriptforge.pyi module
        - provides the static type checking of all the visible interfaces of the ScriptForge API.
        - when the user uses an IDE like PyCharm or VSCode, (s)he might benefit from the typing
          hints provided by them thanks to the twin scriptforge.pyi module.

    Usage:

        When Python and LibreOffice run in the same process (usual case):
            from scriptforge import CreateScriptService

        When Python and LibreOffice are started in separate processes,
        LibreOffice being started from console ... (example for Linux with port = 2024)
            ./soffice --accept='socket,host=localhost,port=2024;urp;'
        then use next statements:
            from scriptforge import CreateScriptService, ScriptForge
            ScriptForge(hostname = 'localhost', port = 2024)

        When the user uses an IDE like PyCharm or VSCode, (s)he might benefit from the typing
        hints provided by them thanks to the twin scriptforge.pyi module.

    Specific documentation about the use of ScriptForge from Python scripts:
        https://help.SnipeOffice.org/latest/en-US/text/sbasic/shared/03/sf_intro.html?DbPAR=BASIC
    """

import uno

import datetime
import time
import os
from typing import TypeVar


class _Singleton(type):
    """
        A Singleton metaclass design pattern
        Credits: « Python in a Nutshell » by Alex Martelli, O'Reilly
        """
    instances = {}

    def __call__(cls, *args, **kwargs):
        if cls not in cls.instances:
            cls.instances[cls] = super(_Singleton, cls).__call__(*args, **kwargs)
        return cls.instances[cls]


# #####################################################################################################################
#                           ScriptForge CLASS                                                                       ###
# #####################################################################################################################

class ScriptForge(object, metaclass = _Singleton):
    """
        The ScriptForge class encapsulates the core of the ScriptForge run-time
            - Bridge with the LibreOffice process
            - Implementation of the inter-language protocol with the Basic libraries
            - Identification of the available services interfaces
            - Dispatching of services
            - Coexistence with UNO

        The class may be instantiated several times. Only the first instance will be retained ("Singleton").

        All its properties and methods are for INTERNAL / DEBUGGING use only.
        """

    # #########################################################################
    # Class attributes
    # #########################################################################
    # Inter-process parameters
    hostname = ''
    port = 0
    pipe = ''
    remoteprocess = False

    componentcontext = None  # com.sun.star.uno.XComponentContext
    scriptprovider = None   # com.sun.star.script.provider.XScriptProvider
    SCRIPTFORGEINITDONE = False  # When True, an instance of the class exists

    servicesdispatcher = None   # com.sun.star.script.provider.XScript to 'basicdispatcher' constant
    serviceslist = {}           # Dictionary of all available services

    # #########################################################################
    # Class constants
    # #########################################################################
    library = 'ScriptForge'
    Version = '25.8'  # Version number of the LibreOffice release containing the actual file
    #
    # Basic dispatcher for Python scripts (@scope#library.module.function)
    basicdispatcher = '@application#ScriptForge.SF_PythonHelper._PythonDispatcher'
    # Python helper functions module
    pythonhelpermodule = 'ScriptForgeHelper.py'     # Preset in production mode,
    #                                                 might be changed (by devs only) in test mode
    #
    # VarType() constants
    V_EMPTY, V_NULL, V_INTEGER, V_LONG, V_SINGLE, V_DOUBLE = 0, 1, 2, 3, 4, 5
    V_CURRENCY, V_DATE, V_STRING, V_OBJECT, V_BOOLEAN = 6, 7, 8, 9, 11
    V_VARIANT, V_ARRAY, V_ERROR, V_UNO = 12, 8192, -1, 16
    # Types of objects returned from Basic
    objMODULE, objCLASS, objDICT, objUNO = 1, 2, 3, 4
    # Special argument symbols
    cstSymEmpty, cstSymNull, cstSymMissing = '+++EMPTY+++', '+++NULL+++', '+++MISSING+++'
    # Predefined references for services implemented as standard Basic modules
    servicesmodules = dict([('ScriptForge.Array', 0),
                            ('ScriptForge.Exception', 1),
                            ('ScriptForge.FileSystem', 2),
                            ('ScriptForge.Platform', 3),
                            ('ScriptForge.Region', 4),
                            ('ScriptForge.Services', 5),
                            ('ScriptForge.Session', 6),
                            ('ScriptForge.String', 7),
                            ('ScriptForge.UI', 8)])

    def __init__(self, hostname = '', port = 0, pipe = ''):
        """
            Because singleton, constructor is executed only once while Python active
            Both arguments are mandatory when Python and LibreOffice run in separate processes
            Otherwise both arguments must be left out.
            :param hostname: probably 'localhost'
            :param port: port number
            :param pipe: pipe name
            """
        ScriptForge.hostname = hostname
        ScriptForge.port = port
        ScriptForge.pipe = pipe
        # Determine main pyuno entry points
        ScriptForge.componentcontext = self.ConnectToLOProcess(hostname, port, pipe)
                                # com.sun.star.uno.XComponentContext
        ScriptForge.remoteprocess = (port > 0 and len(hostname) > 0) or len(pipe) > 0
        ScriptForge.scriptprovider = self.ScriptProvider(ScriptForge.componentcontext)  # ...script.provider.XScriptProvider
        #
        # Establish a list of the available services as a dictionary (servicename, serviceclass)
        ScriptForge.serviceslist = dict((cls.servicename, cls) for cls in SFServices.__subclasses__())
        ScriptForge.servicesdispatcher = None
        #
        # All properties and methods of the ScriptForge API are ProperCased
        # Compute their synonyms as lowercased and camelCased names
        ScriptForge.SetAttributeSynonyms()
        #
        ScriptForge.SCRIPTFORGEINITDONE = True

    @classmethod
    def ConnectToLOProcess(cls, hostname = '', port = 0, pipe = ''):
        """
            Called by the ScriptForge class constructor to establish the connection with
            the requested LibreOffice instance
            The default arguments are for the usual interactive mode

            :param hostname: probably 'localhost' or ''
            :param port: port number or 0
            :param pipe: pipe name or ''
            :return: the derived component context
            """
        if (len(hostname) > 0 and port > 0 and len(pipe) == 0) \
                or (len(hostname) == 0 and port == 0 and len(pipe) > 0):    # Explicit connection via socket or pipe
            ctx = uno.getComponentContext()  # com.sun.star.uno.XComponentContext
            resolver = ctx.ServiceManager.createInstanceWithContext(
                'com.sun.star.bridge.UnoUrlResolver', ctx)  # com.sun.star.comp.bridge.UnoUrlResolver
            try:
                if len(pipe) == 0:
                    conn = 'socket,host=%s,port=%d' % (hostname, port)
                else:
                    conn = 'pipe,name=%s' % pipe
                url = 'uno:%s;urp;StarOffice.ComponentContext' % conn
                ctx = resolver.resolve(url)
            except Exception:  # thrown when LibreOffice specified instance isn't started
                raise SystemExit(
                    "Connection to LibreOffice failed (%s)" % conn)
            return ctx
        elif len(hostname) == 0 and port == 0 and len(pipe) == 0:  # Usual interactive mode
            return uno.getComponentContext()
        else:
            raise SystemExit('The creation of the ScriptForge() instance got invalid arguments: '
                             + "(host = '" + hostname + "', port = " + str(port) + ", pipe = '" + pipe + "')")

    @classmethod
    def ScriptProvider(cls, context = None):
        """
            Returns the general script provider
            """
        servicemanager = context.ServiceManager  # com.sun.star.lang.XMultiComponentFactory
        masterscript = servicemanager.createInstanceWithContext(
            'com.sun.star.script.provider.MasterScriptProviderFactory', context)
        return masterscript.createScriptProvider("")

    @classmethod
    def InvokeSimpleScript(cls, script, *args):
        """
            Low-level script execution via the script provider protocol:
                Create a UNO object corresponding with the given Python or Basic script
                The execution is done with the invoke() method applied on the created object
            Implicit scope: Either
                "application"            a shared library                    (BASIC)
                "share"                  a module within SnipeOffice Macros  (PYTHON)
            :param script: Either
                    [@][scope#][library.]module.method - Must not be a class module or method
                        [@] means that the targeted method accepts ParamArray arguments (Basic only)
                    [scope#][directory/]module.py$method - Must be a method defined at module level
            :return: the value returned by the invoked script without interpretation
                    An error is raised when the script is not found.
            """

        def ParseScript(_script):
            # Check ParamArray, scope, script to run, arguments
            _paramarray = False
            if _script[0] == '@':
                _script = _script[1:]
                _paramarray = True
            scope = ''
            if '#' in _script:
                scope, _script = _script.split('#')
            if '.py$' in _script.lower():  # Python
                if len(scope) == 0:
                    scope = 'share'  # Default for Python
                # Provide an alternate helper script depending on test context
                if _script.startswith(cls.pythonhelpermodule) and hasattr(cls, 'pythonhelpermodule2'):
                    _script = cls.pythonhelpermodule2 + _script[len(cls.pythonhelpermodule):]
                    if '#' in _script:
                        scope, _script = _script.split('#')
                uri = 'vnd.sun.star.script:{0}?language=Python&location={1}'.format(_script, scope)
            else:  # Basic
                if len(scope) == 0:
                    scope = 'application'  # Default for Basic
                lib = ''
                if len(_script.split('.')) < 3:
                    lib = cls.library + '.'  # Default library = ScriptForge
                uri = 'vnd.sun.star.script:{0}{1}?language=Basic&location={2}'.format(lib, _script, scope)
            # Get the script object
            _fullscript = ('@' if _paramarray else '') + scope + '#' + _script
            try:
                _xscript = cls.scriptprovider.getScript(uri)     # com.sun.star.script.provider.XScript
            except Exception:
                raise RuntimeError(
                    'The script \'{0}\' could not be located in your LibreOffice installation'.format(_script))
            return _paramarray, _fullscript, _xscript

        # The frequently called PythonDispatcher in the ScriptForge Basic library is cached to privilege performance
        if cls.servicesdispatcher is not None and script == cls.basicdispatcher:
            xscript = cls.servicesdispatcher
            fullscript = script
            paramarray = True
        # Parse the 'script' argument and build the URI specification described in
        # https://wiki.SnipeOffice.org/Documentation/DevGuide/Scripting_Framework#Scripting_Framework_URI_Specification
        elif len(script) > 0:
            paramarray, fullscript, xscript = ParseScript(script)
        else:  # Should not happen
            return None

        # At 1st execution of the common Basic dispatcher, buffer xscript
        if fullscript == cls.basicdispatcher and cls.servicesdispatcher is None:
            cls.servicesdispatcher = xscript

        # Execute the script with the given arguments
        # Packaging for script provider depends on presence of ParamArray arguments in the called Basic script
        if paramarray:
            scriptreturn = xscript.invoke(args[0], (), ())
        else:
            scriptreturn = xscript.invoke(args, (), ())

        #
        return scriptreturn[0]  # Updatable arguments passed by reference are ignored

    @classmethod
    def InvokeBasicService(cls, basicobject, flags, method, *args):
        """
            High-level script execution via the ScriptForge inter-language protocol:
            To be used for all service methods having their implementation in the Basic world
                Substitute dictionary arguments by sets of UNO property values
                Execute the given Basic method on a class instance
                Interpret its result
            This method has as counterpart the ScriptForge.SF_PythonHelper._PythonDispatcher() Basic method
            :param basicobject: a SFServices subclass instance
                The real object is cached in a Basic Global variable and identified by its reference
            :param flags: see the vb* and flg* constants in the SFServices class
            :param method: the name of the method or property to invoke, as a string
            :param args: the arguments of the method. Symbolic cst* constants may be necessary
            :return: The invoked Basic counterpart script (with InvokeSimpleScript()) will return a tuple
                [0/Value]       The returned value - scalar, object reference, UNO object or a tuple
                [1/VarType]     The Basic VarType() of the returned value
                                Null, Empty and Nothing have own vartypes but return all None to Python
                Additionally, when [0] is a tuple:
                    [2/Dims]        Number of dimensions of the original Basic array
                Additionally, when [0] is a UNO or Basic object:
                    [2/Class]       Basic module (1), Basic class instance (2), Dictionary (3), UNO object (4)
                Additionally, when [0] is a Basic object:
                    [3/Type]        The object's ObjectType
                    [4/Service]     The object's ServiceName
                    [5/Name]        The object's name
                When an error occurs Python receives None as a scalar. This determines the occurrence of a failure
                The method returns either
                    - the 0th element of the tuple when scalar, tuple or UNO object
                    - a new SFServices() object or one of its subclasses otherwise
            """
        # Constants
        script = cls.basicdispatcher
        cstNoArgs = '+++NOARGS+++'
        cstValue, cstVarType, cstDims, cstClass, cstType, cstService, cstName = 0, 1, 2, 2, 3, 4, 5

        def ConvertDictArgs():
            """
                Convert dictionaries in arguments to sets of property values
                """
            argslist = list(args)
            for i in range(len(args)):
                arg = argslist[i]
                if isinstance(arg, dict):
                    argdict = arg
                    if not isinstance(argdict, SFScriptForge.SF_Dictionary):
                        argdict = CreateScriptService('ScriptForge.Dictionary', arg)
                    argslist[i] = argdict.ConvertToPropertyValues()
            return tuple(argslist)

        #
        # Intercept dictionary arguments
        if flags & SFServices.flgDictArg == SFServices.flgDictArg:  # Bits comparison
            args = ConvertDictArgs()
        #
        # Run the basic script
        # The targeted script has a ParamArray argument. Do not change next 4 lines except if you know what you do !
        if len(args) == 0:
            args = (basicobject,) + (flags,) + (method,) + (cstNoArgs,)
        else:
            args = (basicobject,) + (flags,) + (method,) + args
        returntuple = cls.InvokeSimpleScript(script, args)
        #
        # Interpret the result
        # Did an error occur in the Basic world ?
        if not isinstance(returntuple, (tuple, list)):
            raise RuntimeError("The execution of the method '" + method + "' failed. Execution stops.")
        #
        # Analyze the returned tuple
        # Distinguish:
        #   A Basic object to be mapped onto a new Python class instance
        #   A UNO object
        #   A set of property values to be returned as a dict()
        #   An array, tuple or tuple of tuples - manage dates inside
        #   A scalar, Nothing, a date
        returnvalue = returntuple[cstValue]
        if returntuple[cstVarType] == cls.V_OBJECT and len(returntuple) > cstClass:  # Skip Nothing
            if returntuple[cstClass] == cls.objUNO:
                pass
            elif returntuple[cstClass] == cls.objDICT:
                dico = CreateScriptService('ScriptForge.Dictionary')
                if not isinstance(returnvalue, uno.ByteSequence):   # if array not empty
                    dico.ImportFromPropertyValues(returnvalue, overwrite = True)
                return dico
            else:
                # Create the new class instance of the right subclass of SFServices()
                servname = returntuple[cstService]
                if servname not in cls.serviceslist:
                    # When service not found
                    raise RuntimeError("The service '" + servname + "' is not available in Python. Execution stops.")
                subcls = cls.serviceslist[servname]
                if subcls is not None:
                    return subcls(returnvalue, returntuple[cstType], returntuple[cstClass], returntuple[cstName])
        elif returntuple[cstVarType] >= cls.V_ARRAY:
            # Intercept empty array
            if isinstance(returnvalue, uno.ByteSequence):
                return ()
            if flags & SFServices.flgDateRet == SFServices.flgDateRet:  # Bits comparison
                # Intercept all UNO dates in the 1D or 2D array
                if isinstance(returnvalue[0], tuple):   # tuple of tuples
                    arr = []
                    for i in range(len(returnvalue)):
                        row = tuple(map(SFScriptForge.SF_Basic.CDateFromUnoDateTime, returnvalue[i]))
                        arr.append(row)
                    returnvalue = tuple(arr)
                else:                                   # 1D tuple
                    returnvalue = tuple(map(SFScriptForge.SF_Basic.CDateFromUnoDateTime, returnvalue))
        elif returntuple[cstVarType] == cls.V_DATE:
            dat = SFScriptForge.SF_Basic.CDateFromUnoDateTime(returnvalue)
            return dat
        else:  # All other scalar values
            pass
        return returnvalue

    @classmethod
    def initializeRoot(cls, force = False):
        """
            Initialize the global scriptforge data structure.
            When force = False, only when not yet done.
            When force = True, reinitialize it whatever its status.
            """
        script = 'application#ScriptForge.SF_Utils._InitializeRoot'
        return cls.InvokeSimpleScript(script, force)

    @classmethod
    def errorHandling(cls, standard = True):
        """
            Determine how errors in the ScriptForge Basic code are handled. Either
            - the standard mode, i.e. display a "crash" message to the user
            - the debugging mode, i.e. the execution stops on the line causing the error
            """
        script = 'application#ScriptForge.SF_Utils._ErrorHandling'
        return cls.InvokeSimpleScript(script, standard)

    @classmethod
    def SetAttributeSynonyms(cls):
        """
            A synonym of an attribute is either the lowercase or the camelCase form of its original ProperCase name.
            In every subclass of SFServices:
            1) Fill the propertysynonyms dictionary with the synonyms of the properties listed in serviceproperties
                Example:
                     serviceproperties = dict(ConfigFolder = False, InstallFolder = False)
                     propertysynonyms = dict(configfolder = 'ConfigFolder', installfolder = 'InstallFolder',
                                             configFolder = 'ConfigFolder', installFolder = 'InstallFolder')
            2) Define new method attributes synonyms of the original methods
                Example:
                    def CopyFile(...):
                        # etc ...
                    copyFile, copyfile = CopyFile, CopyFile
            """

        def camelCase(key):
            return key[0].lower() + key[1:]

        for cls in SFServices.__subclasses__():
            # Synonyms of properties
            if hasattr(cls, 'serviceproperties'):
                dico = cls.serviceproperties
                dicosyn = dict(zip(map(str.lower, dico.keys()), dico.keys()))  # lower case
                cc = dict(zip(map(camelCase, dico.keys()), dico.keys()))  # camel Case
                dicosyn.update(cc)
                setattr(cls, 'propertysynonyms', dicosyn)
            # Synonyms of methods. A method is a public callable attribute
            methods = [method for method in dir(cls) if not method.startswith('_')]
            for method in methods:
                func = getattr(cls, method)
                if callable(func):
                    # Assign to each synonym a reference to the original method
                    lc = method.lower()
                    setattr(cls, lc, func)
                    cc = camelCase(method)
                    if cc != lc:
                        setattr(cls, cc, func)
        return

    @staticmethod
    def unpack_args(kwargs):
        """
            Convert a dictionary passed as argument to a list alternating keys and values
            Example:
                dict(A = 'a', B = 2) => 'A', 'a', 'B', 2
            """
        return [v for p in zip(list(kwargs.keys()), list(kwargs.values())) for v in p]


# #####################################################################################################################
#                           SFServices CLASS    (ScriptForge services superclass)                                   ###
# #####################################################################################################################

class SFServices(object):
    """
        Generic implementation of a parent Service class.
        Every service must subclass this class to be recognized as a valid service.
        A service instance is created by the CreateScriptService method
        It can have a mirror in the Basic world or be totally defined in Python.

        Every subclass must initialize 3 class properties:
            servicename (e.g. 'ScriptForge.FileSystem', 'ScriptForge.Basic')
            servicesynonyms (e.g. 'FileSystem', 'Basic')
            serviceimplementation: either 'python' or 'basic'
        This is sufficient to register the service in the Python world

        The communication with Basic is managed by 2 ScriptForge() methods:
            InvokeSimpleScript(): low level invocation of a Basic script. This script must be located
                in a usual Basic module. The result is passed as-is
            InvokeBasicService(): the result comes back encapsulated with additional info
                The result is interpreted in the method
                The invoked script can be a property or a method of a Basic class or usual module
        It is up to every service method to determine which method to use

        For Basic services only:
            Each instance is identified by its
                - object reference: the real Basic object embedded as a UNO wrapper object
                - object type ('SF_String', 'DICTIONARY', ...)
                - class module: 1 for usual modules, 2 for class modules
                - name (form, control, ... name) - may be blank

            The role of the SFServices() superclass is mainly to propose a generic properties management
            Properties are got and set following next strategy:
                1. Property names are controlled strictly ('Value' or 'value', not 'VALUE')
                2. Getting a property value for the first time is always done via a Basic call
                3. Next occurrences are fetched from the Python dictionary of the instance if the property
                   is read-only, otherwise via a Basic call

            Each subclass must define its interface with the user scripts:
            1.  The properties
                Property names are proper-cased
                Conventionally, camel-cased and lower-cased synonyms are supported where relevant
                Properties are grouped in a dictionary named 'serviceproperties'
                with keys = (proper-cased) property names and value = int
                    0 = read-only, fetch value locally
                    1 = read-only, fetch value from UNO/Basic because value might have been changed by user
                    2 = editable, fetch value locally
                    3 = editable, fetch value from UNO/Basic because value might have been changed by user
                Properties that may be fetched locally are buffered in Python after their 1st get request to Basic
                or after their update.
                If there is a need to handle a specific property in a specific manner:
                    @property
                    def myProperty(self):
                        return self.GetProperty('myProperty')
            2   The methods
                a usual def: statement
                    def myMethod(self, arg1, arg2 = ''):
                        return self.Execute(self.vbMethod, 'myMethod', arg1, arg2)
                Method names are proper-cased, arguments are lower-cased
                Conventionally, camel-cased and lower-cased homonyms are supported in method names where relevant
                All arguments must be present and initialized before the call to Basic, if any
        """
    # Python-Basic protocol constants and flags
    vbGet, vbLet, vbMethod, vbSet = 2, 4, 1, 8  # CallByName constants
    flgPost = 16  # The method or the property implies a hardcoded post-processing
    flgDictArg = 32  # Invoked service method may contain a dict argument
    flgDateArg = 64  # Invoked service method may contain a date argument
    flgDateRet = 128  # Invoked service method can return a date
    flgArrayArg = 512  # 1st argument can be a 2D array
    flgArrayRet = 1024  # Invoked service method can return a 2D array (standard modules) or any array (class modules)
    flgUno = 256  # Invoked service method/property can return a UNO object
    flgObject = 2048  # 1st argument may be a Basic object
    flgHardCode = 4096  # Force hardcoded call to method, avoid CallByName()
    # Basic class type
    moduleClass, moduleStandard = 2, 1
    #
    # Empty dictionary for lower/camelcased homonyms of properties
    propertysynonyms = {}
    # To operate dynamic property getting/setting it is necessary to
    # enumerate all types of properties and adapt __getattr__() and __setattr__() according to their type
    internal_attributes = ('objectreference', 'objecttype', 'name', 'servicename',
                           'serviceimplementation', 'classmodule', 'EXEC', 'SIMPLEEXEC')
    # Shortcuts to script provider interfaces
    SIMPLEEXEC = ScriptForge.InvokeSimpleScript
    EXEC = ScriptForge.InvokeBasicService

    def __init__(self, reference = -1, objtype = None, classmodule = 0, name = ''):
        """
            Trivial initialization of internal properties
            If the subclass has its own __init()__ method, a call to this one should be its first statement.
            """
        self.objectreference = reference  # the index in the Python storage where the Basic object is stored
        self.objecttype = objtype  # ('SF_String', 'TIMER', ...)
        self.classmodule = classmodule  # Module (1), Class instance (2)
        self.name = name  # '' when no name

    def __getattr__(self, name):
        """
            Executed for EVERY property reference if name not yet in the instance dict
            At the 1st get, the property value is always got from Basic
            Due to the use of lower/camelcase synonyms, it is called for each variant of the same property
            The method manages itself the buffering in __dict__ based on the official ProperCase property name
            """
        if name in self.propertysynonyms:  # Reset real name if argument provided in lower or camel case
            name = self.propertysynonyms[name]
        if self.serviceimplementation == 'basic':
            if name in ('serviceproperties', 'internal_attributes', 'propertysynonyms'):
                pass
            elif name in self.serviceproperties:
                prop = self.GetProperty(name)   # Get Property from Basic
                if self.serviceproperties[name] in (0, 2):  # Store the property value for later re-use
                    object.__setattr__(self, name, prop)
                return prop
        # Execute the usual attributes getter
        return super(SFServices, self).__getattribute__(name)

    def __setattr__(self, name, value):
        """
            Executed for EVERY property assignment, including in __init__() !!
            Setting a property required for all serviceproperties() to be executed in Basic
            The new value is stored for re-use in the local instance when relevant
            """
        if self.serviceimplementation == 'basic':
            if name in self.internal_attributes:
                pass
            elif name in self.serviceproperties or name in self.propertysynonyms:
                if name in self.propertysynonyms:  # Reset real name if argument provided in lower or camel case
                    name = self.propertysynonyms[name]
                proplevel = self.serviceproperties[name]
                if proplevel in (2, 3):  # Editable
                    self.SetProperty(name, value)
                    if proplevel == 3:  # Do not store in the local instance
                        return
                else:
                    raise AttributeError(
                        "object of type '" + self.objecttype + "' has no editable property '" + name + "'")
            else:
                raise AttributeError("object of type '" + self.objecttype + "' has no property '" + name + "'")
        object.__setattr__(self, name, value)   # Store the new value in the local instance
        return

    def __repr__(self):
        return self.serviceimplementation + '/' + self.servicename + '/' + str(self.objectreference) + '/' + \
               super(SFServices, self).__repr__()

    def Dispose(self):
        if self.serviceimplementation == 'basic':
            if self.objectreference >= len(ScriptForge.servicesmodules):  # Do not dispose predefined module objects
                self.ExecMethod(self.vbMethod + self.flgPost, 'Dispose')
                self.objectreference = -1

    def ExecMethod(self, flags = 0, methodname = '', *args):
        if flags == 0:
            flags = self.vbMethod
        if len(methodname) > 0:
            return self.EXEC(self.objectreference, flags, methodname, *args)

    def GetProperty(self, propertyname, arg = None):
        """
            Get the given property from the Basic world
            """
        if self.serviceimplementation == 'basic':
            # Conventionally properties starting with X (and only them) may return a UNO object
            calltype = self.vbGet + (self.flgUno if propertyname[0] == 'X' else 0)
            if arg is None:
                return self.EXEC(self.objectreference, calltype, propertyname)
            else:  # There are a few cases (Calc ...) where GetProperty accepts an argument
                return self.EXEC(self.objectreference, calltype, propertyname, arg)
        return None

    def Properties(self):
        return list(self.serviceproperties)

    def basicmethods(self):
        if self.serviceimplementation == 'basic':
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'Methods')
        else:
            return []

    def basicproperties(self):
        if self.serviceimplementation == 'basic':
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'Properties')
        else:
            return []

    def SetProperty(self, propertyname, value):
        """
            Set the given property to a new value in the Basic world
            """
        if self.serviceimplementation == 'basic':
            flag = self.vbLet
            if isinstance(value, datetime.datetime):
                value = SFScriptForge.SF_Basic.CDateToUnoDateTime(value)
                flag += self.flgDateArg
            elif isinstance(value, dict):
                flag += self.flgDictArg
            if repr(type(value)) == "<class 'pyuno'>":
                flag += self.flgUno
            return self.EXEC(self.objectreference, flag, propertyname, value)


# #####################################################################################################################
#                       SFScriptForge CLASS    (alias of ScriptForge Basic library)                                 ###
# #####################################################################################################################
class SFScriptForge:

    # #########################################################################
    # SF_Array CLASS
    # #########################################################################
    class SF_Array(SFServices, metaclass = _Singleton):
        """
            Provides a collection of methods for manipulating and transforming arrays of one dimension (vectors)
            and arrays of two dimensions (matrices). This includes set operations, sorting,
            importing to and exporting from text files.
            The Python version of the service provides a single method: ImportFromCSVFile
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'ScriptForge.Array'
        servicesynonyms = ('array', 'scriptforge.array')
        serviceproperties = dict()

        def ImportFromCSVFile(self, filename, delimiter = ',', dateformat = ''):
            """
                Difference with the Basic version: dates are returned in their iso format,
                not as any of the datetime objects.
                """
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'ImportFromCSVFile',
                                   filename, delimiter, dateformat)

    # #########################################################################
    # SF_Basic CLASS
    # #########################################################################
    class SF_Basic(SFServices, metaclass = _Singleton):
        """
            This service proposes a collection of Basic methods to be executed in a Python context
            simulating the exact syntax and behaviour of the identical Basic builtin method.
            Typical example:
                SF_Basic.MsgBox('This has to be displayed in a message box')

            The signatures of Basic builtin functions are derived from
                core/basic/source/runtime/stdobj.cxx

            Detailed user documentation:
                https://help.SnipeOffice.org/latest/en-US/text/sbasic/shared/03/sf_basic.html?DbPAR=BASIC
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'python'
        servicename = 'ScriptForge.Basic'
        servicesynonyms = ('basic', 'scriptforge.basic')
        # Basic helper functions invocation
        module = 'SF_PythonHelper'
        # Message box constants
        MB_ABORTRETRYIGNORE, MB_DEFBUTTON1, MB_DEFBUTTON2, MB_DEFBUTTON3 = 2, 128, 256, 512
        MB_ICONEXCLAMATION, MB_ICONINFORMATION, MB_ICONQUESTION, MB_ICONSTOP = 48, 64, 32, 16
        MB_OK, MB_OKCANCEL, MB_RETRYCANCEL, MB_YESNO, MB_YESNOCANCEL = 0, 1, 5, 4, 3
        IDABORT, IDCANCEL, IDIGNORE, IDNO, IDOK, IDRETRY, IDYES = 3, 2, 5, 7, 1, 4, 6

        @classmethod
        def CDate(cls, datevalue):
            cdate = cls.SIMPLEEXEC(cls.module + '.PyCDate', datevalue)
            return cls.CDateFromUnoDateTime(cdate)

        @staticmethod
        def CDateFromUnoDateTime(unodate):
            """
                Converts a UNO date/time representation to a datetime.datetime Python native object
                :param unodate: com.sun.star.util.DateTime, com.sun.star.util.Date or com.sun.star.util.Time
                :return: the equivalent datetime.datetime
                """
            date = datetime.datetime(1899, 12, 30, 0, 0, 0, 0)  # Idem as Basic builtin TimeSerial() function
            datetype = repr(type(unodate))
            if 'com.sun.star.util.DateTime' in datetype:
                if 1900 <= unodate.Year <= datetime.MAXYEAR:
                    date = datetime.datetime(unodate.Year, unodate.Month, unodate.Day, unodate.Hours,
                                             unodate.Minutes, unodate.Seconds, int(unodate.NanoSeconds / 1000))
            elif 'com.sun.star.util.Date' in datetype:
                if 1900 <= unodate.Year <= datetime.MAXYEAR:
                    date = datetime.datetime(unodate.Year, unodate.Month, unodate.Day)
            elif 'com.sun.star.util.Time' in datetype:
                date = datetime.datetime(unodate.Hours, unodate.Minutes, unodate.Seconds,
                                         int(unodate.NanoSeconds / 1000))
            else:
                return unodate  # Not recognized as a UNO date structure
            return date

        @staticmethod
        def CDateToUnoDateTime(date):
            """
                Converts a date representation into the ccom.sun.star.util.DateTime date format
                Acceptable boundaries: year >= 1900 and <= 32767
                :param date: datetime.datetime, datetime.date, datetime.time, float (time.time) or time.struct_time
                :return: a com.sun.star.util.DateTime
                """
            unodate = uno.createUnoStruct('com.sun.star.util.DateTime')
            unodate.Year, unodate.Month, unodate.Day, unodate.Hours, unodate.Minutes, unodate.Seconds, \
                unodate.NanoSeconds, unodate.IsUTC = \
                1899, 12, 30, 0, 0, 0, 0, False  # Identical to Basic TimeSerial() function

            if isinstance(date, float):
                date = time.localtime(date)
            if isinstance(date, time.struct_time):
                if 1900 <= date[0] <= 32767:
                    unodate.Year, unodate.Month, unodate.Day, unodate.Hours, unodate.Minutes, unodate.Seconds = \
                        date[0:6]
                else:  # Copy only the time related part
                    unodate.Hours, unodate.Minutes, unodate.Seconds = date[3:3]
            elif isinstance(date, (datetime.datetime, datetime.date, datetime.time)):
                if isinstance(date, (datetime.datetime, datetime.date)):
                    if 1900 <= date.year <= 32767:
                        unodate.Year, unodate.Month, unodate.Day = date.year, date.month, date.day
                if isinstance(date, (datetime.datetime, datetime.time)):
                    unodate.Hours, unodate.Minutes, unodate.Seconds, unodate.NanoSeconds = \
                        date.hour, date.minute, date.second, date.microsecond * 1000
            else:
                return date  # Not recognized as a date
            return unodate

        @classmethod
        def ConvertFromUrl(cls, url):
            return cls.SIMPLEEXEC(cls.module + '.PyConvertFromUrl', url)

        @classmethod
        def ConvertToUrl(cls, systempath):
            return cls.SIMPLEEXEC(cls.module + '.PyConvertToUrl', systempath)

        @classmethod
        def CreateUnoService(cls, servicename):
            return cls.SIMPLEEXEC(cls.module + '.PyCreateUnoService', servicename)

        @classmethod
        def CreateUnoStruct(cls, unostructure):
            return uno.createUnoStruct(unostructure)

        @classmethod
        def DateAdd(cls, interval, number, date):
            if isinstance(date, datetime.datetime):
                date = cls.CDateToUnoDateTime(date)
            dateadd = cls.SIMPLEEXEC(cls.module + '.PyDateAdd', interval, number, date)
            return cls.CDateFromUnoDateTime(dateadd)

        @classmethod
        def DateDiff(cls, interval, date1, date2, firstdayofweek = 1, firstweekofyear = 1):
            if isinstance(date1, datetime.datetime):
                date1 = cls.CDateToUnoDateTime(date1)
            if isinstance(date2, datetime.datetime):
                date2 = cls.CDateToUnoDateTime(date2)
            return cls.SIMPLEEXEC(cls.module + '.PyDateDiff', interval, date1, date2, firstdayofweek, firstweekofyear)

        @classmethod
        def DatePart(cls, interval, date, firstdayofweek = 1, firstweekofyear = 1):
            if isinstance(date, datetime.datetime):
                date = cls.CDateToUnoDateTime(date)
            return cls.SIMPLEEXEC(cls.module + '.PyDatePart', interval, date, firstdayofweek, firstweekofyear)

        @classmethod
        def DateValue(cls, string):
            if isinstance(string, datetime.datetime):
                string = string.isoformat()
            datevalue = cls.SIMPLEEXEC(cls.module + '.PyDateValue', string)
            return cls.CDateFromUnoDateTime(datevalue)

        @classmethod
        def Format(cls, expression, format = ''):
            if isinstance(expression, datetime.datetime):
                expression = cls.CDateToUnoDateTime(expression)
            return cls.SIMPLEEXEC(cls.module + '.PyFormat', expression, format)

        @classmethod
        def GetDefaultContext(cls):
            return ScriptForge.componentcontext

        @classmethod
        def GetGuiType(cls):
            return cls.SIMPLEEXEC(cls.module + '.PyGetGuiType')

        @classmethod
        def GetPathSeparator(cls):
            return os.sep

        @classmethod
        def GetSystemTicks(cls):
            return cls.SIMPLEEXEC(cls.module + '.PyGetSystemTicks')

        class GlobalScope(object, metaclass = _Singleton):
            @classmethod  # Mandatory because the GlobalScope class is normally not instantiated
            def BasicLibraries(cls):
                return ScriptForge.InvokeSimpleScript(SFScriptForge.SF_Basic.module + '.PyGlobalScope', 'Basic')

            @classmethod
            def DialogLibraries(cls):
                return ScriptForge.InvokeSimpleScript(SFScriptForge.SF_Basic.module + '.PyGlobalScope', 'Dialog')

        @classmethod
        def InputBox(cls, prompt, title = '', default = '', xpostwips = -1, ypostwips = -1):
            if xpostwips < 0 or ypostwips < 0:
                return cls.SIMPLEEXEC(cls.module + '.PyInputBox', prompt, title, default)
            return cls.SIMPLEEXEC(cls.module + '.PyInputBox', prompt, title, default, xpostwips, ypostwips)

        @classmethod
        def MsgBox(cls, prompt, buttons = 0, title = ''):
            return cls.SIMPLEEXEC(cls.module + '.PyMsgBox', prompt, buttons, title)

        @classmethod
        def Now(cls):
            return datetime.datetime.now()

        @classmethod
        def RGB(cls, red, green, blue):
            return int('%02x%02x%02x' % (red, green, blue), 16)

        @property
        def StarDesktop(self):
            ctx = ScriptForge.componentcontext
            if ctx is None:
                return None
            smgr = ctx.getServiceManager()  # com.sun.star.lang.XMultiComponentFactory
            DESK = 'com.sun.star.frame.Desktop'
            desktop = smgr.createInstanceWithContext(DESK, ctx)
            return desktop

        starDesktop, stardesktop = StarDesktop, StarDesktop

        @property
        def ThisComponent(self):
            """
                When the current component is the Basic IDE, the ThisComponent object returns
                in Basic the component owning the currently run user script.
                Above behaviour cannot be reproduced in Python.
                :return: the current component or None when not a document
                """
            comp = self.StarDesktop.getCurrentComponent()
            if comp is None:
                return None
            impl = comp.ImplementationName
            if impl in ('com.sun.star.comp.basic.BasicIDE', 'com.sun.star.comp.sfx2.BackingComp'):
                return None  # None when Basic IDE or welcome screen
            return comp

        thisComponent, thiscomponent = ThisComponent, ThisComponent

        @property
        def ThisDatabaseDocument(self):
            """
                When the current component is the Basic IDE, the ThisDatabaseDocument object returns
                in Basic the database owning the currently run user script.
                Above behaviour cannot be reproduced in Python.
                :return: the current Base (main) component or None when not a Base document or one of its subcomponents
            """
            comp = self.ThisComponent  # Get the current component
            if comp is None:
                return None
            #
            sess = CreateScriptService('Session')
            impl, ident = '', ''
            if sess.HasUnoProperty(comp, 'ImplementationName'):
                impl = comp.ImplementationName
            if sess.HasUnoProperty(comp, 'Identifier'):
                ident = comp.Identifier
            #
            targetimpl = 'com.sun.star.comp.dba.ODatabaseDocument'
            if impl == targetimpl:  # The current component is the main Base window
                return comp
            # Identify resp. form, table/query, table/query in edit mode, report, relations diagram
            if impl == 'SwXTextDocument' and ident == 'com.sun.star.sdb.FormDesign' \
                    or impl == 'org.openoffice.comp.dbu.ODatasourceBrowser' \
                    or impl in ('org.openoffice.comp.dbu.OTableDesign', 'org.openoffice.comp.dbu.OQuertDesign') \
                    or impl == 'SwXTextDocument' and ident == 'com.sun.star.sdb.TextReportDesign' \
                    or impl == 'org.openoffice.comp.dbu.ORelationDesign':
                db = comp.ScriptContainer
                if sess.HasUnoProperty(db, 'ImplementationName'):
                    if db.ImplementationName == targetimpl:
                        return db
            return None

        thisDatabaseDocument, thisdatabasedocument = ThisDatabaseDocument, ThisDatabaseDocument

        @classmethod
        def Xray(cls, unoobject = None):
            return cls.SIMPLEEXEC('XrayTool._main.xray', unoobject)

    # #########################################################################
    # SF_Dictionary CLASS
    # #########################################################################
    class SF_Dictionary(SFServices, dict):
        """
            The service adds to a Python dict instance the interfaces for conversion to and from
            a list of UNO PropertyValues

            Usage:
                dico = dict(A = 1, B = 2, C = 3)
                myDict = CreateScriptService('Dictionary', dico)    # Initialize myDict with the content of dico
                myDict['D'] = 4
                print(myDict)   # {'A': 1, 'B': 2, 'C': 3, 'D': 4}
                propval = myDict.ConvertToPropertyValues()
            or
                dico = dict(A = 1, B = 2, C = 3)
                myDict = CreateScriptService('Dictionary')          # Initialize myDict as an empty dict object
                myDict.update(dico) # Load the values of dico into myDict
                myDict['D'] = 4
                print(myDict)   # {'A': 1, 'B': 2, 'C': 3, 'D': 4}
                propval = myDict.ConvertToPropertyValues()
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'python'
        servicename = 'ScriptForge.Dictionary'
        servicesynonyms = ('dictionary', 'scriptforge.dictionary')

        def __init__(self, dic = None):
            SFServices.__init__(self)
            dict.__init__(self)
            if dic is not None:
                self.update(dic)

        def ConvertToPropertyValues(self):
            """
                Store the content of the dictionary in an array of PropertyValues.
                Each entry in the array is a com.sun.star.beans.PropertyValue.
                he key is stored in Name, the value is stored in Value.

                If one of the items has a type datetime, it is converted to a com.sun.star.util.DateTime structure.
                If one of the items is an empty list, it is converted to None.

                The resulting array is empty when the dictionary is empty.
                """
            result = []
            for key in iter(self):
                value = self[key]
                item = value
                if isinstance(value, dict):  # check that first level is not itself a (sub)dict
                    item = None
                elif isinstance(value, (tuple, list)):  # check every member of the list is not a (sub)dict
                    if len(value) == 0:  # Property values do not like empty lists
                        value = None
                    else:
                        for i in range(len(value)):
                            if isinstance(value[i], dict):
                                value[i] = None
                    item = value
                elif isinstance(value, (datetime.datetime, datetime.date, datetime.time)):
                    item = SFScriptForge.SF_Basic.CDateToUnoDateTime(value)
                pv = uno.createUnoStruct('com.sun.star.beans.PropertyValue')
                pv.Name = key
                pv.Value = item
                result.append(pv)
            return result

        def ImportFromPropertyValues(self, propertyvalues, overwrite = False):
            """
                Inserts the contents of an array of PropertyValue objects into the current dictionary.
                PropertyValue Names are used as keys in the dictionary, whereas Values contain the corresponding values.
                Date-type values are converted to datetime.datetime instances.
                :param propertyvalues: a list.tuple containing com.sun.star.beans.PropertyValue objects
                :param overwrite: When True, entries with same name may exist in the dictionary and their values
                    are overwritten. When False (default), repeated keys are not overwritten.
                :return: True when successful
                """
            result = []
            for pv in iter(propertyvalues):
                key = pv.Name
                if overwrite is True or key not in self:
                    item = pv.Value
                    if 'com.sun.star.util.DateTime' in repr(type(item)):
                        item = datetime.datetime(item.Year, item.Month, item.Day,
                                                 item.Hours, item.Minutes, item.Seconds, int(item.NanoSeconds / 1000))
                    elif 'com.sun.star.util.Date' in repr(type(item)):
                        item = datetime.datetime(item.Year, item.Month, item.Day)
                    elif 'com.sun.star.util.Time' in repr(type(item)):
                        item = datetime.datetime(item.Hours, item.Minutes, item.Seconds, int(item.NanoSeconds / 1000))
                    result.append((key, item))
            self.update(result)
            return True

    # #########################################################################
    # SF_Exception CLASS
    # #########################################################################
    class SF_Exception(SFServices, metaclass = _Singleton):
        """
            The Exception service is a collection of methods for code debugging and error handling.

            The Exception service console stores events, variable values and information about errors.
            Use the console when the Python shell is not available, for example in Calc user defined functions (UDF)
            or during events processing.
            Use DebugPrint() method to aggregate additional user data of any type.

            Console entries can be dumped to a text file or visualized in a dialogue.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'ScriptForge.Exception'
        servicesynonyms = ('exception', 'scriptforge.exception')
        serviceproperties = dict(ReportScriptErrors = 3, ReturnCode = 1, ReturnCodeDescription = 1, StopWhenError = 3)

        def Clear(self):
            return self.ExecMethod(self.vbMethod, 'Clear')

        def Console(self, modal = True):
            # From Python, the current XComponentContext must be added as last argument
            return self.ExecMethod(self.vbMethod, 'Console', modal, ScriptForge.componentcontext)

        def ConsoleClear(self, keep = 0):
            return self.ExecMethod(self.vbMethod, 'ConsoleClear', keep)

        def ConsoleToFile(self, filename):
            return self.ExecMethod(self.vbMethod, 'ConsoleToFile', filename)

        def DebugDisplay(self, *args):
            # Arguments are concatenated in a single string similar to what the Python print() function would produce
            self.DebugPrint(*args)
            param = '\n'.join(list(map(lambda a: a.strip("'") if isinstance(a, str) else repr(a), args)))
            bas = CreateScriptService('ScriptForge.Basic')
            return bas.MsgBox(param, bas.MB_OK + bas.MB_ICONINFORMATION, 'DebugDisplay')

        def DebugPrint(self, *args):
            # Arguments are concatenated in a single string similar to what the Python print() function would produce
            # Avoid using repr() on strings to not have backslashes * 4
            param = '\t'.join(list(map(lambda a: a.strip("'") if isinstance(a, str) else repr(a),
                                       args))).expandtabs(tabsize = 4)
            return self.ExecMethod(self.vbMethod, 'DebugPrint', param)

        @classmethod
        def PythonShell(cls, variables = None, background = 0xFDF6E3, foreground = 0x657B83):
            """
                Open an APSO python shell window - Thanks to its authors Hanya/Tsutomu Uchino/Hubert Lambert
                :param variables: Typical use
                                        PythonShell.({**globals(), **locals()})
                                  to push the global and local dictionaries to the shell window
                :param background: background color as an int
                :param foreground: foreground color as an int
                """
            if variables is None:
                variables = locals()
            # Is APSO installed ?
            ctx = ScriptForge.componentcontext
            ext = ctx.getByName('/singletons/com.sun.star.deployment.PackageInformationProvider')
            apso = 'apso.python.script.organizer'
            if len(ext.getPackageLocation(apso)) > 0:
                # APSO is available. However, PythonShell() is ignored in bridge mode
                # because APSO library is not in pythonpath
                if ScriptForge.remoteprocess:
                    return None
                # Directly derived from apso.oxt|python|scripts|tools.py$console
                # we need to load apso before import statement
                ctx.ServiceManager.createInstance('apso.python.script.organizer.impl')
                # now we can use apso_utils library
                from apso_utils import console
                kwargs = {'loc': variables, 'BACKGROUND': background, 'FOREGROUND': foreground, 'prettyprint': False}
                kwargs['loc'].setdefault('XSCRIPTCONTEXT', uno)
                console(**kwargs)
                # An interprocess call is necessary to allow a redirection of STDOUT and STDERR by APSO
                #   Choice is a minimalist call to a Basic routine: no arguments, a few lines of code
                SFScriptForge.SF_Basic.GetGuiType()
            else:
                # The APSO extension could not be located in your LibreOffice installation
                cls._RaiseFatal('SF_Exception.PythonShell', 'variables=None', 'PYTHONSHELLERROR')

        @classmethod
        def RaiseFatal(cls, errorcode, *args):
            """
                Generate a run-time error caused by an anomaly in a user script detected by ScriptForge
                The message is logged in the console. The execution is STOPPED
                For INTERNAL USE only
                """
            # Direct call because RaiseFatal forces an execution stop in Basic
            if len(args) == 0:
                args = (None,)
            return cls.SIMPLEEXEC('@SF_Exception.RaiseFatal', (errorcode, *args))  # With ParamArray

        @classmethod
        def _RaiseFatal(cls, sub, subargs, errorcode, *args):
            """
                Wrapper of RaiseFatal(). Includes method and syntax of the failed Python routine
                to simulate the exact behaviour of the Basic RaiseFatal() method
                For INTERNAL USE only
                """
            ScriptForge.InvokeSimpleScript('ScriptForge.SF_Utils._EnterFunction', sub, subargs)
            cls.RaiseFatal(errorcode, *args)
            raise RuntimeError("The execution of the method '" + sub.split('.')[-1] + "' failed. Execution stops.")

    # #########################################################################
    # SF_FileSystem CLASS
    # #########################################################################
    class SF_FileSystem(SFServices, metaclass = _Singleton):
        """
            The "FileSystem" service includes common file and folder handling routines.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'ScriptForge.FileSystem'
        servicesynonyms = ('filesystem', 'scriptforge.filesystem')
        serviceproperties = dict(ConfigFolder = 1, ExtensionsFolder = 1, FileNaming = 3, HomeFolder = 1,
                                 InstallFolder = 1, TemplatesFolder = 1, TemporaryFolder = 1,
                                 UserTemplatesFolder = 1) # 1 because FileNaming determines every time the folder format
        # Open TextStream constants
        ForReading, ForWriting, ForAppending = 1, 2, 8

        def BuildPath(self, foldername, name):
            return self.ExecMethod(self.vbMethod, 'BuildPath', foldername, name)

        def CompareFiles(self, filename1, filename2, comparecontents = False):
            py = ScriptForge.pythonhelpermodule + '$' + '_SF_FileSystem__CompareFiles'
            if self.FileExists(filename1) and self.FileExists(filename2):
                file1 = self._ConvertFromUrl(filename1)
                file2 = self._ConvertFromUrl(filename2)
                return self.SIMPLEEXEC(py, file1, file2, comparecontents)
            else:
                return False

        def CopyFile(self, source, destination, overwrite = True):
            return self.ExecMethod(self.vbMethod, 'CopyFile', source, destination, overwrite)

        def CopyFolder(self, source, destination, overwrite = True):
            return self.ExecMethod(self.vbMethod, 'CopyFolder', source, destination, overwrite)

        def CreateFolder(self, foldername):
            return self.ExecMethod(self.vbMethod, 'CreateFolder', foldername)

        def CreateTextFile(self, filename, overwrite = True, encoding = 'UTF-8'):
            return self.ExecMethod(self.vbMethod, 'CreateTextFile', filename, overwrite, encoding)

        def DeleteFile(self, filename):
            return self.ExecMethod(self.vbMethod, 'DeleteFile', filename)

        def DeleteFolder(self, foldername):
            return self.ExecMethod(self.vbMethod, 'DeleteFolder', foldername)

        def ExtensionFolder(self, extension):
            return self.ExecMethod(self.vbMethod, 'ExtensionFolder', extension)

        def FileExists(self, filename):
            return self.ExecMethod(self.vbMethod, 'FileExists', filename)

        def Files(self, foldername, filter = '', includesubfolders = False):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'Files', foldername, filter, includesubfolders)

        def FolderExists(self, foldername):
            return self.ExecMethod(self.vbMethod, 'FolderExists', foldername)

        def GetBaseName(self, filename):
            return self.ExecMethod(self.vbMethod, 'GetBaseName', filename)

        def GetExtension(self, filename):
            return self.ExecMethod(self.vbMethod, 'GetExtension', filename)

        def GetFileLen(self, filename):
            py = ScriptForge.pythonhelpermodule + '$' + '_SF_FileSystem__GetFilelen'
            if self.FileExists(filename):
                file = self._ConvertFromUrl(filename)
                return int(self.SIMPLEEXEC(py, file))
            else:
                return 0

        def GetFileModified(self, filename):
            return self.ExecMethod(self.vbMethod + self.flgDateRet, 'GetFileModified', filename)

        def GetName(self, filename):
            return self.ExecMethod(self.vbMethod, 'GetName', filename)

        def GetParentFolderName(self, filename):
            return self.ExecMethod(self.vbMethod, 'GetParentFolderName', filename)

        def GetTempName(self, extension = ''):
            return self.ExecMethod(self.vbMethod, 'GetTempName', extension)

        def HashFile(self, filename, algorithm):
            py = ScriptForge.pythonhelpermodule + '$' + '_SF_FileSystem__HashFile'
            if self.FileExists(filename):
                file = self._ConvertFromUrl(filename)
                return self.SIMPLEEXEC(py, file, algorithm.lower())
            else:
                return ''

        def MoveFile(self, source, destination):
            return self.ExecMethod(self.vbMethod, 'MoveFile', source, destination)

        def MoveFolder(self, source, destination):
            return self.ExecMethod(self.vbMethod, 'MoveFolder', source, destination)

        def Normalize(self, filename):
            return self.ExecMethod(self.vbMethod, 'Normalize', filename)

        def OpenTextFile(self, filename, iomode = 1, create = False, encoding = 'UTF-8'):
            return self.ExecMethod(self.vbMethod, 'OpenTextFile', filename, iomode, create, encoding)

        def PickFile(self, defaultfile = ScriptForge.cstSymEmpty, mode = 'OPEN', filter = ''):
            return self.ExecMethod(self.vbMethod, 'PickFile', defaultfile, mode, filter)

        def PickFolder(self, defaultfolder = ScriptForge.cstSymEmpty, freetext = ''):
            return self.ExecMethod(self.vbMethod, 'PickFolder', defaultfolder, freetext)

        def SubFolders(self, foldername, filter = '', includesubfolders = False):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'SubFolders', foldername,
                                   filter, includesubfolders)

        @classmethod
        def _ConvertFromUrl(cls, filename):
            # Alias for same function in FileSystem Basic module
            return cls.SIMPLEEXEC('ScriptForge.SF_FileSystem._ConvertFromUrl', filename)

    # #########################################################################
    # SF_L10N CLASS
    # #########################################################################
    class SF_L10N(SFServices):
        """
            This service provides a number of methods related to the translation of strings
            with minimal impact on the program's source code.
            The methods provided by the L10N service can be used mainly to:
                Create POT files that can be used as templates for translation of all strings in the program.
                Get translated strings at runtime for the language defined in the Locale property.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'ScriptForge.L10N'
        servicesynonyms = ('l10n', 'scriptforge.l10n')
        serviceproperties = dict(Folder = 0, Languages = 0, Locale = 0)

        @classmethod
        def ReviewServiceArgs(cls, foldername = '', locale = '', encoding = 'UTF-8',
                              locale2 = '', encoding2 = 'UTF-8'):
            """
                Transform positional and keyword arguments into positional only
                """
            return foldername, locale, encoding, locale2, encoding2

        def AddText(self, context = '', msgid = '', comment = ''):
            return self.ExecMethod(self.vbMethod, 'AddText', context, msgid, comment)

        def AddTextsFromDialog(self, dialog):
            dialogobj = dialog.objectreference if isinstance(dialog, SFDialogs.SF_Dialog) else dialog
            return self.ExecMethod(self.vbMethod + self.flgObject, 'AddTextsFromDialog', dialogobj)

        def ExportToPOTFile(self, filename, header = '', encoding = 'UTF-8'):
            return self.ExecMethod(self.vbMethod, 'ExportToPOTFile', filename, header, encoding)

        def GetText(self, msgid, *args):
            return self.ExecMethod(self.vbMethod, 'GetText', msgid, *args)

        _ = GetText

    # #########################################################################
    # SF_Platform CLASS
    # #########################################################################
    class SF_Platform(SFServices, metaclass = _Singleton):
        """
            The 'Platform' service implements a collection of properties about the actual execution environment
            and context :
                the hardware platform
                the operating system
                the LibreOffice version
                the current user
            All those properties are read-only.
            The implementation is mainly based on the 'platform' module of the Python standard library
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'ScriptForge.Platform'
        servicesynonyms = ('platform', 'scriptforge.platform')
        serviceproperties = dict(Extensions = 0, FilterNames = 0, Fonts = 0, FormatLocale = 0,
                                 Locale = 0, OfficeLocale = 0, OfficeVersion = 0,
                                 Printers = 0, SystemLocale = 0,UserData = 0)
        # Python helper functions
        py = ScriptForge.pythonhelpermodule + '$' + '_SF_Platform'

        @property
        def Architecture(self):
            return self.SIMPLEEXEC(self.py, 'Architecture')

        architecture = Architecture

        @property
        def ComputerName(self):
            return self.SIMPLEEXEC(self.py, 'ComputerName')

        computername, computerName = ComputerName, ComputerName

        @property
        def CPUCount(self):
            return self.SIMPLEEXEC(self.py, 'CPUCount')

        cpucount, cpuCount = CPUCount, CPUCount

        @property
        def CurrentUser(self):
            return self.SIMPLEEXEC(self.py, 'CurrentUser')

        currentuser, currentUser = CurrentUser, CurrentUser

        @property
        def Machine(self):
            return self.SIMPLEEXEC(self.py, 'Machine')

        machine = Machine

        @property
        def OSName(self):
            return self.SIMPLEEXEC(self.py, 'OSName')

        osname, osName = OSName, OSName

        @property
        def OSPlatform(self):
            return self.SIMPLEEXEC(self.py, 'OSPlatform')

        osplatform, osPlatform = OSPlatform, OSPlatform

        @property
        def OSRelease(self):
            return self.SIMPLEEXEC(self.py, 'OSRelease')

        osrelease, osRelease = OSRelease, OSRelease

        @property
        def OSVersion(self):
            return self.SIMPLEEXEC(self.py, 'OSVersion')

        osversion, osVersion = OSVersion, OSVersion

        @property
        def Processor(self):
            return self.SIMPLEEXEC(self.py, 'Processor')

        processor = Processor

        @property
        def PythonVersion(self):
            return self.SIMPLEEXEC(self.py, 'PythonVersion')

        pythonversion, pythonVersion = PythonVersion, PythonVersion

        @property
        def UntitledPrefix(self):
            basic = SFScriptForge.SF_Basic()
            desktop = basic.StarDesktop
            return desktop.UntitledPrefix

        untitledprefix, untitledPrefix = UntitledPrefix, UntitledPrefix

    # #########################################################################
    # SF_Region CLASS
    # #########################################################################
    class SF_Region(SFServices, metaclass = _Singleton):
        """
            The "Region" service gathers a collection of functions about languages, countries and timezones
                - Locales
                - Currencies
                - Numbers and dates formatting
                - Calendars
                - Timezones conversions
                - Numbers transformed to text
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'ScriptForge.Region'
        servicesynonyms = ('region', 'scriptforge.region')
        serviceproperties = dict()

        # Next functions are implemented in Basic as read-only properties with 1 argument
        def Country(self, region = ''):
            return self.GetProperty('Country', region)

        def Currency(self, region = ''):
            return self.GetProperty('Currency', region)

        def DatePatterns(self, region = ''):
            return self.GetProperty('DatePatterns', region)

        def DateSeparator(self, region = ''):
            return self.GetProperty('DateSeparator', region)

        def DayAbbrevNames(self, region = ''):
            return self.GetProperty('DayAbbrevNames', region)

        def DayNames(self, region = ''):
            return self.GetProperty('DayNames', region)

        def DayNarrowNames(self, region = ''):
            return self.GetProperty('DayNarrowNames', region)

        def DecimalPoint(self, region = ''):
            return self.GetProperty('DecimalPoint', region)

        def Language(self, region = ''):
            return self.GetProperty('Language', region)

        def ListSeparator(self, region = ''):
            return self.GetProperty('ListSeparator', region)

        def MonthAbbrevNames(self, region = ''):
            return self.GetProperty('MonthAbbrevNames', region)

        def MonthNames(self, region = ''):
            return self.GetProperty('MonthNames', region)

        def MonthNarrowNames(self, region = ''):
            return self.GetProperty('MonthNarrowNames', region)

        def ThousandSeparator(self, region = ''):
            return self.GetProperty('ThousandSeparator', region)

        def TimeSeparator(self, region = ''):
            return self.GetProperty('TimeSeparator', region)

        # Usual methods
        def DSTOffset(self, localdatetime, timezone, locale = ''):
            if isinstance(localdatetime, datetime.datetime):
                localdatetime = SFScriptForge.SF_Basic.CDateToUnoDateTime(localdatetime)
            return self.ExecMethod(self.vbMethod + self.flgDateArg, 'DSTOffset', localdatetime, timezone, locale)

        def LocalDateTime(self, utcdatetime, timezone, locale = ''):
            if isinstance(utcdatetime, datetime.datetime):
                utcdatetime = SFScriptForge.SF_Basic.CDateToUnoDateTime(utcdatetime)
            localdate = self.ExecMethod(self.vbMethod + self.flgDateArg + self.flgDateRet, 'LocalDateTime',
                                        utcdatetime, timezone, locale)
            return SFScriptForge.SF_Basic.CDateFromUnoDateTime(localdate)

        def Number2Text(self, number, locale = ''):
            return self.ExecMethod(self.vbMethod, 'Number2Text', number, locale)

        def TimeZoneOffset(self, timezone, locale = ''):
            return self.ExecMethod(self.vbMethod, 'TimeZoneOffset', timezone, locale)

        def UTCDateTime(self, localdatetime, timezone, locale = ''):
            if isinstance(localdatetime, datetime.datetime):
                localdatetime = SFScriptForge.SF_Basic.CDateToUnoDateTime(localdatetime)
            utcdate = self.ExecMethod(self.vbMethod + self.flgDateArg + self.flgDateRet, 'UTCDateTime', localdatetime,
                                      timezone, locale)
            return SFScriptForge.SF_Basic.CDateFromUnoDateTime(utcdate)

        def UTCNow(self, timezone, locale = ''):
            now = self.ExecMethod(self.vbMethod + self.flgDateRet, 'UTCNow', timezone, locale)
            return SFScriptForge.SF_Basic.CDateFromUnoDateTime(now)

    # #########################################################################
    # SF_Session CLASS
    # #########################################################################
    class SF_Session(SFServices, metaclass = _Singleton):
        """
            The Session service gathers various general-purpose methods about:
            - UNO introspection
            - the invocation of external scripts or programs
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'ScriptForge.Session'
        servicesynonyms = ('session', 'scriptforge.session')
        serviceproperties = dict()

        # Class constants                       Where to find an invoked library ?
        SCRIPTISEMBEDDED = 'document'  # in the document
        SCRIPTISAPPLICATION = 'application'  # in any shared library (Basic)
        SCRIPTISPERSONAL = 'user'  # in My Macros (Python)
        SCRIPTISPERSOXT = 'user:uno_packages'  # in an extension installed for the current user (Python)
        SCRIPTISSHARED = 'share'  # in SnipeOffice macros (Python)
        SCRIPTISSHAROXT = 'share:uno_packages'  # in an extension installed for all users (Python)
        SCRIPTISOXT = 'uno_packages'  # in an extension but the installation parameters are unknown (Python)

        @classmethod
        def ExecuteBasicScript(cls, scope = '', script = '', *args):
            if scope is None or scope == '':
                scope = cls.SCRIPTISAPPLICATION
            if len(args) == 0:
                args = (scope,) + (script,) + (None,)
            else:
                args = (scope,) + (script,) + args
            # ExecuteBasicScript method has a ParamArray parameter in Basic
            return cls.SIMPLEEXEC('@SF_Session.ExecuteBasicScript', args)

        @classmethod
        def ExecuteCalcFunction(cls, calcfunction, *args):
            if len(args) == 0:
                # Arguments of Calc functions are strings or numbers. None == Empty is a good alias for no argument
                args = (calcfunction,) + (None,)
            else:
                # Date arguments are converted on-the-fly to com.sun.star.util.DateTime
                args = (calcfunction,) + tuple(map(SFScriptForge.SF_Basic.CDateToUnoDateTime, args))
            # ExecuteCalcFunction method has a ParamArray parameter in Basic
            return cls.SIMPLEEXEC('@SF_Session.ExecuteCalcFunction', args)

        @classmethod
        def ExecutePythonScript(cls, scope = '', script = '', *args):
            return cls.SIMPLEEXEC(scope + '#' + script, *args)

        def GetPDFExportOptions(self):
            return self.ExecMethod(self.vbMethod, 'GetPDFExportOptions')

        def HasUnoMethod(self, unoobject, methodname):
            return self.ExecMethod(self.vbMethod, 'HasUnoMethod', unoobject, methodname)

        def HasUnoProperty(self, unoobject, propertyname):
            return self.ExecMethod(self.vbMethod, 'HasUnoProperty', unoobject, propertyname)

        @classmethod
        def OpenURLInBrowser(cls, url):
            py = ScriptForge.pythonhelpermodule + '$' + '_SF_Session__OpenURLInBrowser'
            return cls.SIMPLEEXEC(py, url)

        def RunApplication(self, command, parameters):
            return self.ExecMethod(self.vbMethod, 'RunApplication', command, parameters)

        def SendMail(self, recipient, cc = '', bcc = '', subject = '', body = '', filenames = '', editmessage = True):
            return self.ExecMethod(self.vbMethod, 'SendMail', recipient, cc, bcc, subject, body, filenames, editmessage)

        def SetPDFExportOptions(self, pdfoptions):
            return self.ExecMethod(self.vbMethod + self.flgDictArg, 'SetPDFExportOptions', pdfoptions)

        def UnoMethods(self, unoobject):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'UnoMethods', unoobject)

        def UnoObjectType(self, unoobject):
            return self.ExecMethod(self.vbMethod, 'UnoObjectType', unoobject)

        def UnoProperties(self, unoobject):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'UnoProperties', unoobject)

        def WebService(self, uri):
            return self.ExecMethod(self.vbMethod, 'WebService', uri)

    # #########################################################################
    # SF_String CLASS
    # #########################################################################
    class SF_String(SFServices, metaclass = _Singleton):
        """
            Focus on string manipulation, regular expressions, encodings and hashing algorithms.
            The methods implemented in Basic that are redundant with Python builtin functions
            are not duplicated
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'ScriptForge.String'
        servicesynonyms = ('string', 'scriptforge.string')
        serviceproperties = dict()

        @classmethod
        def HashStr(cls, inputstr, algorithm):
            py = ScriptForge.pythonhelpermodule + '$' + '_SF_String__HashStr'
            return cls.SIMPLEEXEC(py, inputstr, algorithm.lower())

        def IsADate(self, inputstr, dateformat = 'YYYY-MM-DD'):
            return self.ExecMethod(self.vbMethod, 'IsADate', inputstr, dateformat)

        def IsEmail(self, inputstr):
            return self.ExecMethod(self.vbMethod, 'IsEmail', inputstr)

        def IsFileName(self, inputstr, osname = ScriptForge.cstSymEmpty):
            return self.ExecMethod(self.vbMethod, 'IsFileName', inputstr, osname)

        def IsIBAN(self, inputstr):
            return self.ExecMethod(self.vbMethod, 'IsIBAN', inputstr)

        def IsIPv4(self, inputstr):
            return self.ExecMethod(self.vbMethod, 'IsIPv4', inputstr)

        def IsLike(self, inputstr, pattern, casesensitive = False):
            return self.ExecMethod(self.vbMethod, 'IsLike', inputstr, pattern, casesensitive)

        def IsSheetName(self, inputstr):
            return self.ExecMethod(self.vbMethod, 'IsSheetName', inputstr)

        def IsUrl(self, inputstr):
            return self.ExecMethod(self.vbMethod, 'IsUrl', inputstr)

        def SplitNotQuoted(self, inputstr, delimiter = ' ', occurrences = 0, quotechar = '"'):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'SplitNotQuoted', inputstr, delimiter,
                                   occurrences, quotechar)

        def Wrap(self, inputstr, width = 70, tabsize = 8):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'Wrap', inputstr, width, tabsize)

    # #########################################################################
    # SF_TextStream CLASS
    # #########################################################################
    class SF_TextStream(SFServices):
        """
            The TextStream service is used to sequentially read from and write to files opened or created
            using the ScriptForge.FileSystem service..
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'ScriptForge.TextStream'
        servicesynonyms = ()
        serviceproperties = dict(AtEndOfStream = 1, Encoding = 0, FileName = 0, IOMode = 0, Line = 1, NewLine = 2)

        def CloseFile(self):
            return self.ExecMethod(self.vbMethod, 'CloseFile')

        def ReadAll(self):
            return self.ExecMethod(self.vbMethod, 'ReadAll')

        def ReadLine(self):
            return self.ExecMethod(self.vbMethod, 'ReadLine')

        def SkipLine(self):
            return self.ExecMethod(self.vbMethod, 'SkipLine')

        def WriteBlankLines(self, lines):
            return self.ExecMethod(self.vbMethod, 'WriteBlankLines', lines)

        def WriteLine(self, line):
            return self.ExecMethod(self.vbMethod, 'WriteLine', line)

    # #########################################################################
    # SF_Timer CLASS
    # #########################################################################
    class SF_Timer(SFServices):
        """
            The "Timer" service measures the amount of time it takes to run user scripts.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'ScriptForge.Timer'
        servicesynonyms = ('timer', 'scriptforge.timer')
        serviceproperties = dict(Duration = 1, IsStarted = 1, IsSuspended = 1,
                                 SuspendDuration = 1, TotalDuration = 1)

        @classmethod
        def ReviewServiceArgs(cls, start = False):
            """
                Transform positional and keyword arguments into positional only
                """
            return (start,)

        def Continue(self):
            return self.ExecMethod(self.vbMethod, 'Continue')

        def Restart(self):
            return self.ExecMethod(self.vbMethod, 'Restart')

        def Start(self):
            return self.ExecMethod(self.vbMethod, 'Start')

        def Suspend(self):
            return self.ExecMethod(self.vbMethod, 'Suspend')

        def Terminate(self):
            return self.ExecMethod(self.vbMethod, 'Terminate')

    # #########################################################################
    # SF_UI CLASS
    # #########################################################################
    class SF_UI(SFServices, metaclass = _Singleton):
        """
            Singleton class for the identification and the manipulation of the
            different windows composing the whole LibreOffice application:
                - Windows selection
                - Windows moving and resizing
                - Statusbar settings
                - Creation of new windows
                - Access to the underlying "documents"
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'ScriptForge.UI'
        servicesynonyms = ('ui', 'scriptforge.ui')
        serviceproperties = dict(ActiveWindow = 1, Height = 1, Width = 1, X = 1, Y = 1)

        # Class constants
        MACROEXECALWAYS, MACROEXECNEVER, MACROEXECNORMAL = 2, 1, 0
        BASEDOCUMENT, CALCDOCUMENT, DRAWDOCUMENT, IMPRESSDOCUMENT, MATHDOCUMENT, WRITERDOCUMENT = \
            'Base', 'Calc', 'Draw', 'Impress', 'Math', 'Writer'

        @property
        def ActiveWindow(self):
            return self.ExecMethod(self.vbMethod, 'ActiveWindow')

        def Activate(self, windowname = ''):
            return self.ExecMethod(self.vbMethod, 'Activate', windowname)

        def CreateBaseDocument(self, filename, embeddeddatabase = 'HSQLDB', registrationname = '', calcfilename = ''):
            return self.ExecMethod(self.vbMethod, 'CreateBaseDocument', filename, embeddeddatabase, registrationname,
                                   calcfilename)

        def CreateDocument(self, documenttype = '', templatefile = '', hidden = False):
            return self.ExecMethod(self.vbMethod, 'CreateDocument', documenttype, templatefile, hidden)

        def Documents(self):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'Documents')

        def GetDocument(self, windowname = ''):
            return self.ExecMethod(self.vbMethod, 'GetDocument', windowname)

        def Maximize(self, windowname = ''):
            return self.ExecMethod(self.vbMethod, 'Maximize', windowname)

        def Minimize(self, windowname = ''):
            return self.ExecMethod(self.vbMethod, 'Minimize', windowname)

        def OpenBaseDocument(self, filename = '', registrationname = '', macroexecution = MACROEXECNORMAL):
            return self.ExecMethod(self.vbMethod, 'OpenBaseDocument', filename, registrationname, macroexecution)

        def OpenDocument(self, filename, password = '', readonly = False, hidden = False,
                         macroexecution = MACROEXECNORMAL, filtername = '', filteroptions = ''):
            return self.ExecMethod(self.vbMethod, 'OpenDocument', filename, password, readonly, hidden,
                                   macroexecution, filtername, filteroptions)

        def Resize(self, left = -1, top = -1, width = -1, height = -1):
            return self.ExecMethod(self.vbMethod, 'Resize', left, top, width, height)

        def RunCommand(self, command, *args, **kwargs):
            params = tuple(list(args) + ScriptForge.unpack_args(kwargs))
            if len(params) == 0:
                params = (command,) + (None,)
            else:
                params = (command,) + params
            return self.SIMPLEEXEC('@SF_UI.RunCommand', params)

        def SetStatusbar(self, text = '', percentage = -1):
            return self.ExecMethod(self.vbMethod, 'SetStatusbar', text, percentage)

        def ShowProgressBar(self, title = '', text = '', percentage = -1):
            # From Python, the current XComponentContext must be added as last argument
            return self.ExecMethod(self.vbMethod, 'ShowProgressBar', title, text, percentage,
                                   ScriptForge.componentcontext)

        def WindowExists(self, windowname):
            return self.ExecMethod(self.vbMethod, 'WindowExists', windowname)


# #####################################################################################################################
#                       SFDatabases CLASS    (alias of SFDatabases Basic library)                                   ###
# #####################################################################################################################
class SFDatabases:
    """
        The SFDatabases class manages databases embedded in or connected to Base documents
        """
    pass

    # #########################################################################
    # SF_Database CLASS
    # #########################################################################
    class SF_Database(SFServices):
        """
            Each instance of the current class represents a single database, with essentially its tables, queries
            and data
            The exchanges with the database are done in SQL only.
            To make them more readable, use optionally square brackets to surround table/query/field names
            instead of the (RDBMS-dependent) normal surrounding character.
            SQL statements may be run in direct or indirect mode. In direct mode the statement is transferred literally
            without syntax checking nor review to the database engine.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFDatabases.Database'
        servicesynonyms = ('database', 'sfdatabases.database')
        serviceproperties = dict(Queries = 0, Tables = 0, XConnection = 0, XMetaData = 0)

        @classmethod
        def ReviewServiceArgs(cls, filename = '', registrationname = '', readonly = True, user = '', password = ''):
            """
                Transform positional and keyword arguments into positional only
                """
            return filename, registrationname, readonly, user, password

        def CloseDatabase(self):
            return self.ExecMethod(self.vbMethod, 'CloseDatabase')

        def Commit(self):
            return self.ExecMethod(self.vbMethod, 'Commit')

        def CreateDataset(self, sqlcommand, directsql = False, filter = '', orderby = ''):
            return self.ExecMethod(self.vbMethod, 'CreateDataset', sqlcommand, directsql, filter, orderby)

        def DAvg(self, expression, tablename, criteria = ''):
            return self.ExecMethod(self.vbMethod, 'DAvg', expression, tablename, criteria)

        def DCount(self, expression, tablename, criteria = ''):
            return self.ExecMethod(self.vbMethod, 'DCount', expression, tablename, criteria)

        def DLookup(self, expression, tablename, criteria = '', orderclause = ''):
            return self.ExecMethod(self.vbMethod, 'DLookup', expression, tablename, criteria, orderclause)

        def DMax(self, expression, tablename, criteria = ''):
            return self.ExecMethod(self.vbMethod + self.flgDateRet, 'DMax', expression, tablename, criteria)

        def DMin(self, expression, tablename, criteria = ''):
            return self.ExecMethod(self.vbMethod + self.flgDateRet, 'DMin', expression, tablename, criteria)

        def DSum(self, expression, tablename, criteria = ''):
            return self.ExecMethod(self.vbMethod, 'DSum', expression, tablename, criteria)

        def GetRows(self, sqlcommand, directsql = False, header = False, maxrows = 0):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet + self.flgDateRet, 'GetRows', sqlcommand,
                                   directsql, header, maxrows)

        def OpenFormDocument(self, formdocument):
            return self.ExecMethod(self.vbMethod, 'OpenFormDocument', formdocument)

        def OpenQuery(self, queryname):
            return self.ExecMethod(self.vbMethod, 'OpenQuery', queryname)

        def OpenSql(self, sql, directsql = False):
            return self.ExecMethod(self.vbMethod, 'OpenSql', sql, directsql)

        def OpenTable(self, tablename):
            return self.ExecMethod(self.vbMethod, 'OpenTable', tablename)

        def Rollback(self):
            return self.ExecMethod(self.vbMethod, 'Rollback')

        def RunSql(self, sqlcommand, directsql = False):
            return self.ExecMethod(self.vbMethod, 'RunSql', sqlcommand, directsql)

        def SetTransactionMode(self, transactionmode = 0):
            return self.ExecMethod(self.vbMethod, 'SetTransactionMode', transactionmode)

    # #########################################################################
    # SF_Dataset CLASS
    # #########################################################################
    class SF_Dataset(SFServices):
        """
            A dataset represents a set of tabular data produced by a database.
            In the user interface of LibreOffice a dataset corresponds with the data
            displayed in a form, a data sheet (table, query).
            To use datasets, the database instance must exist but the Base document may not be open.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFDatabases.Dataset'
        servicesynonyms = ()    # CreateScriptService is not applicable here
        serviceproperties = dict(BOF = 3, DefaultValues = 0, EOF = 3, Fields = 0, Filter = 0,
                                 OrderBy = 0, ParentDatabase = 0, RowCount = 1, RowNumber = 1,
                                 Source = 0, SourceType = 0, UpdatableFields = 0, Values = 1,
                                 XRowSet = 0)

        @classmethod
        def _dictargs(cls, args, kwargs):
            """
                Convert a set of keyword arguments to a dictionary to pass to the Basic world
                """
            if len(args) == 0 and len(kwargs) > 0:
                return kwargs
            if len(args) > 0:
                if len(kwargs) == 0:
                    if isinstance(args[0], dict):
                        return args[0]
                    return {args[i]: args[i + 1] for i in range(0, len(args), 2)}
            return None

        def CloseDataset(self):
            return self.ExecMethod(self.vbMethod, 'CloseDataset')

        def CreateDataset(self, filter = ScriptForge.cstSymMissing, orderby = ScriptForge.cstSymMissing):
            return self.ExecMethod(self.vbMethod, 'CreateDataset', filter, orderby)

        def Delete(self):
            return self.ExecMethod(self.vbMethod, 'Delete')

        def ExportValueToFile(self, fieldname, filename, overwrite = False):
            return self.ExecMethod(self.vbMethod, 'ExportValueToFile', fieldname, filename, overwrite)

        def GetRows(self, header = False, maxrows = 0):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet + self.flgDateRet, 'GetRows', header, maxrows)

        def GetValue(self, fieldname):
            return self.ExecMethod(self.vbMethod, 'GetValue', fieldname)

        def Insert(self, *args, **kwargs):
            updateslist = self._dictargs(args, kwargs)
            if updateslist is None:
                return -1   # The insertion could not be done
            return self.ExecMethod(self.vbMethod + self.flgDictArg, 'Insert', updateslist)

        def MoveFirst(self):
            return self.ExecMethod(self.vbMethod, 'MoveFirst')

        def MoveLast(self):
            return self.ExecMethod(self.vbMethod, 'MoveLast')

        def MoveNext(self, offset = 1):
            return self.ExecMethod(self.vbMethod, 'MoveNext', offset)

        def MovePrevious(self, offset = 1):
            return self.ExecMethod(self.vbMethod, 'MovePrevious', offset)

        def Reload(self, filter = ScriptForge.cstSymMissing, orderby = ScriptForge.cstSymMissing):
            return self.ExecMethod(self.vbMethod, 'Reload', filter, orderby)

        def Update(self, *args, **kwargs):
            updateslist = self._dictargs(args, kwargs)
            if updateslist is None:
                return False   # The update could not be done
            return self.ExecMethod(self.vbMethod + self.flgDictArg, 'Update', updateslist)

    # #########################################################################
    # SF_Datasheet CLASS
    # #########################################################################
    class SF_Datasheet(SFServices):
        """
            A datasheet is the visual representation of tabular data produced by a database.
            A datasheet may be opened automatically by script code at any moment.
            The Base document owning the data may or may not be opened.
            Any SELECT SQL statement may trigger the datasheet display.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFDatabases.Datasheet'
        servicesynonyms = ('datasheet', 'sfdatabases.datasheet')
        serviceproperties = dict(ColumnHeaders = 0, CurrentColumn = 1, CurrentRow = 1,
                                 DatabaseFileName = 0, Filter = 2, IsAlive = 1, LastRow = 0, MenuHeaders = 1,
                                 OrderBy = 2, ParentDatabase = 0, Source = 0, SourceType = 0, XComponent = 0,
                                 XControlModel = 0, XTabControllerModel = 0)

        def Activate(self):
            return self.ExecMethod(self.vbMethod, 'Activate')

        def CloseDatasheet(self):
            return self.ExecMethod(self.vbMethod, 'CloseDatasheet')

        def CreateMenu(self, menuheader, before = '', submenuchar = '>'):
            return self.ExecMethod(self.vbMethod, 'CreateMenu', menuheader, before, submenuchar)

        def GetText(self, column = 0):
            return self.ExecMethod(self.vbMethod, 'GetText', column)

        def GetValue(self, column = 0):
            return self.ExecMethod(self.vbMethod, 'GetValue', column)

        def GoToCell(self, row = 0, column = 0):
            return self.ExecMethod(self.vbMethod, 'GoToCell', row, column)

        def RemoveMenu(self, menuheader):
            return self.ExecMethod(self.vbMethod, 'RemoveMenu', menuheader)

        def Toolbars(self, toolbarname = ''):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'Toolbars', toolbarname)


# #####################################################################################################################
#                       SFDialogs CLASS    (alias of SFDialogs Basic library)                                       ###
# #####################################################################################################################
class SFDialogs:
    """
        The SFDialogs class manages dialogs defined with the Basic IDE
        """
    pass

    # #########################################################################
    # SF_Dialog CLASS
    # #########################################################################
    class SF_Dialog(SFServices):
        """
            Each instance of the current class represents a single dialog box displayed to the user.
            The dialog box must have been designed and defined with the Basic IDE previously.
            From a Python script, a dialog box can be displayed in modal or in non-modal modes.

            In modal mode, the box is displayed and the execution of the macro process is suspended
            until one of the OK or Cancel buttons is pressed. In the meantime, other user actions
            executed on the box can trigger specific actions.

            In non-modal mode, the floating dialog remains displayed until the dialog is terminated
            by code (Terminate()) or until the LibreOffice application stops.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFDialogs.Dialog'
        servicesynonyms = ('dialog', 'sfdialogs.dialog')
        serviceproperties = dict(Caption = 2, Height = 2, IsAlive = 1, Modal = 0, Name = 0,
                                 OnFocusGained = 2, OnFocusLost = 2, OnKeyPressed = 2,
                                 OnKeyReleased = 2, OnMouseDragged = 2, OnMouseEntered = 2,
                                 OnMouseExited = 2, OnMouseMoved = 2, OnMousePressed = 2, OnMouseReleased = 2,
                                 Page = 2, Visible = 2, Width = 2, XDialogModel = 0, XDialogView = 0)
        # Class constants used together with the Execute() method
        OKBUTTON, CANCELBUTTON = 1, 0

        @classmethod
        def ReviewServiceArgs(cls, container = '', library = 'Standard', dialogname = ''):
            """
                Transform positional and keyword arguments into positional only
                Add the XComponentContext as last argument
                """
            return container, library, dialogname, ScriptForge.componentcontext

        # Methods potentially executed while the dialog is in execution require the flgHardCode flag
        def Activate(self):
            return self.ExecMethod(self.vbMethod, 'Activate')

        def Center(self, parent = ScriptForge.cstSymMissing):
            parentclasses = (SFDocuments.SF_Document, SFDocuments.SF_Base, SFDocuments.SF_Calc, SFDocuments.SF_Writer,
                             SFDialogs.SF_Dialog)
            parentobj = parent.objectreference if isinstance(parent, parentclasses) else parent
            return self.ExecMethod(self.vbMethod + self.flgObject, 'Center', parentobj)

        def CloneControl(self, sourcename, controlname, left = 1, top = 1):
            return self.ExecMethod(self.vbMethod, 'CloneControl', sourcename, controlname, left, top)

        def Controls(self, controlname = ''):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet + self.flgHardCode, 'Controls', controlname)

        def CreateButton(self, controlname, place, toggle = False, push = ''):
            return self.ExecMethod(self.vbMethod, 'CreateButton', controlname, place, toggle, push)

        def CreateCheckBox(self, controlname, place, multiline = False):
            return self.ExecMethod(self.vbMethod, 'CreateCheckBox', controlname, place, multiline)

        def CreateComboBox(self, controlname, place, border = '3D', dropdown = True, linecount = 5):
            return self.ExecMethod(self.vbMethod, 'CreateComboBox', controlname, place, border, dropdown, linecount)

        def CreateCurrencyField(self, controlname, place, border = '3D', spinbutton = False, minvalue = -1000000,
                                maxvalue = +1000000, increment = 1, accuracy = 2):
            return self.ExecMethod(self.vbMethod, 'CreateCurrencyField', controlname, place, border, spinbutton,
                                   minvalue, maxvalue, increment, accuracy)

        def CreateDateField(self, controlname, place, border = '3D', dropdown = True,
                            mindate = datetime.datetime(1900, 1, 1, 0, 0, 0, 0),
                            maxdate = datetime.datetime(2200, 12, 31, 0, 0, 0, 0)):
            if isinstance(mindate, datetime.datetime):
                mindate = SFScriptForge.SF_Basic.CDateToUnoDateTime(mindate)
            if isinstance(maxdate, datetime.datetime):
                maxdate = SFScriptForge.SF_Basic.CDateToUnoDateTime(maxdate)
            return self.ExecMethod(self.vbMethod + self.flgDateArg, 'CreateDateField', controlname, place, border,
                                   dropdown, mindate, maxdate)

        def CreateFileControl(self, controlname, place, border = '3D'):
            return self.ExecMethod(self.vbMethod, 'CreateFileControl', controlname, place, border)

        def CreateFixedLine(self, controlname, place, orientation):
            return self.ExecMethod(self.vbMethod, 'CreateFixedLine', controlname, place, orientation)

        def CreateFixedText(self, controlname, place, border = 'NONE', multiline = False, align = 'LEFT',
                            verticalalign = 'TOP'):
            return self.ExecMethod(self.vbMethod, 'CreateFixedText', controlname, place, border, multiline, align,
                                   verticalalign)

        def CreateFormattedField(self, controlname, place, border = '3D', spinbutton = False,
                                 minvalue = -1000000, maxvalue = +1000000):
            return self.ExecMethod(self.vbMethod, 'CreateFormattedField', controlname, place, border, spinbutton,
                                   minvalue, maxvalue)

        def CreateGroupBox(self, controlname, place):
            return self.ExecMethod(self.vbMethod, 'CreateGroupBox', controlname, place)

        def CreateHyperlink(self, controlname, place, border = 'NONE', multiline = False, align = 'LEFT',
                            verticalalign = 'TOP'):
            return self.ExecMethod(self.vbMethod, 'CreateHyperlink', controlname, place, border, multiline, align,
                                   verticalalign)

        def CreateImageControl(self, controlname, place, border = '3D', scale = 'FITTOSIZE'):
            return self.ExecMethod(self.vbMethod, 'CreateImageControl', controlname, place, border, scale)

        def CreateListBox(self, controlname, place, border = '3D', dropdown = True, linecount = 5,
                          multiselect = False):
            return self.ExecMethod(self.vbMethod, 'CreateListBox', controlname, place, border, dropdown,
                                   linecount, multiselect)

        def CreateNumericField(self, controlname, place, border = '3D', spinbutton = False,
                               minvalue = -1000000, maxvalue = +1000000, increment = 1, accuracy = 2):
            return self.ExecMethod(self.vbMethod, 'CreateNumericField', controlname, place, border, spinbutton,
                                   minvalue, maxvalue, increment, accuracy)

        def CreatePatternField(self, controlname, place, border = '3D', editmask = '', literalmask = ''):
            return self.ExecMethod(self.vbMethod, 'CreatePatternField', controlname, place, border,
                                   editmask, literalmask)

        def CreateProgressBar(self, controlname, place, border = '3D', minvalue = 0, maxvalue = 100):
            return self.ExecMethod(self.vbMethod, 'CreateProgressBar', controlname, place, border, minvalue, maxvalue)

        def CreateRadioButton(self, controlname, place, multiline = False):
            return self.ExecMethod(self.vbMethod, 'CreateRadioButton', controlname, place, multiline)

        def CreateScrollBar(self, controlname, place, orientation, border = '3D', minvalue = 0, maxvalue = 100):
            return self.ExecMethod(self.vbMethod, 'CreateScrollBar', controlname, place, orientation, border,
                                   minvalue, maxvalue)

        def CreateTableControl(self, controlname, place, border = '3D', rowheaders = True, columnheaders = True,
                               scrollbars = 'None', gridlines = False):
            return self.ExecMethod(self.vbMethod, 'CreateTableControl', controlname, place, border,
                                   rowheaders, columnheaders, scrollbars, gridlines)

        def CreateTabPageContainer(self, controlname, place, tabheaders, border = '3D'):
            return self.ExecMethod(self.vbMethod, 'CreateTabPageContainer', controlname, place, tabheaders, border)

        def CreateTextField(self, controlname, place, border = '3D', multiline = False,
                            maximumlength = 0, passwordcharacter = ''):
            return self.ExecMethod(self.vbMethod, 'CreateTextField', controlname, place, border,
                                   multiline, maximumlength, passwordcharacter)

        def CreateTimeField(self, controlname, place, border = '3D',
                            mintime = datetime.datetime(1899, 12, 30, 0, 0, 0, 0),
                            maxtime = datetime.datetime(1899, 12, 30, 23, 59, 59, 0)):
            if isinstance(mintime, datetime.datetime):
                mintime = SFScriptForge.SF_Basic.CDateToUnoDateTime(mintime)
            if isinstance(maxtime, datetime.datetime):
                maxtime = SFScriptForge.SF_Basic.CDateToUnoDateTime(maxtime)
            return self.ExecMethod(self.vbMethod + self.flgDateArg, 'CreateTimeField', controlname, place, border,
                                   mintime, maxtime)

        def CreateTreeControl(self, controlname, place, border = '3D'):
            return self.ExecMethod(self.vbMethod, 'CreateTreeControl', controlname, place, border)

        def EndExecute(self, returnvalue):
            return self.ExecMethod(self.vbMethod, 'EndExecute', returnvalue)

        def Execute(self, modal = True):
            return self.ExecMethod(self.vbMethod + self.flgHardCode, 'Execute', modal)

        def GetTextsFromL10N(self, l10n):
            l10nobj = l10n.objectreference if isinstance(l10n, SFScriptForge.SF_L10N) else l10n
            return self.ExecMethod(self.vbMethod + self.flgObject, 'GetTextsFromL10N', l10nobj)

        def ImportControl(self, sourcedialog, sourcename, controlname = '', page = 0, offsetx = 0, offsety = 0,
                          includeonproperties = False):
            dialogobj = sourcedialog.objectreference if isinstance(sourcedialog, SFDialogs.SF_Dialog) else sourcedialog
            return self.ExecMethod(self.vbMethod + self.flgObject, 'ImportControl', dialogobj, sourcename,
                                   controlname, page, offsetx, offsety, includeonproperties)

        def OrderTabs(self, tabslist, start = 1, increment = 1):
            return self.ExecMethod(self.vbMethod, 'OrderTabs', tabslist, start, increment)

        def Resize(self, left = -99999, top = -99999, width = -1, height = -1):
            return self.ExecMethod(self.vbMethod, 'Resize', left, top, width, height)

        def SetPageManager(self, pilotcontrols = '', tabcontrols = '', wizardcontrols = '', lastpage = 0):
            return self.ExecMethod(self.vbMethod, 'SetPageManager', pilotcontrols, tabcontrols, wizardcontrols,
                                   lastpage)

        def Terminate(self):
            return self.ExecMethod(self.vbMethod, 'Terminate')

    # #########################################################################
    # SF_NewDialog CLASS
    # #########################################################################
    class SF_NewDialog(SFServices):
        """
            Pseudo service never returned from the Basic world. A SF_Dialog instance is returned instead.
            Main purpose: manage the arguments of CreateScriptService() for the creation of a dialog from scratch
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFDialogs.NewDialog'
        servicesynonyms = ('newdialog', 'sfdialogs.newdialog')
        serviceproperties = dict()

        @classmethod
        def ReviewServiceArgs(cls, dialogname = '', place = (0, 0, 0, 0)):
            """
                Transform positional and keyword arguments into positional only
                Add the XComponentContext as last argument
                """
            if ScriptForge.remoteprocess:
                return dialogname, place, ScriptForge.componentcontext
            else:
                return dialogname, place

    # #########################################################################
    # SF_DialogControl CLASS
    # #########################################################################
    class SF_DialogControl(SFServices):
        """
            Each instance of the current class represents a single control within a dialog box.
            The focus is clearly set on getting and setting the values displayed by the controls of the dialog box,
            not on their formatting.
            A special attention is given to controls with type TreeControl.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFDialogs.DialogControl'
        servicesynonyms = ()
        serviceproperties = dict(Border = 2, Cancel = 2, Caption = 2, ControlType = 0, CurrentNode = 3,
                                 Default = 2, Enabled = 2, Format = 2, Height = 2, ListCount = 0,
                                 ListIndex = 3, Locked = 2, MultiSelect = 2, Name = 0,
                                 OnActionPerformed = 2, OnAdjustmentValueChanged = 2, OnFocusGained = 2,
                                 OnFocusLost = 2, OnItemStateChanged = 2, OnKeyPressed = 2,
                                 OnKeyReleased = 2, OnMouseDragged = 2, OnMouseEntered = 2,
                                 OnMouseExited = 2, OnMouseMoved = 2, OnMousePressed = 2,
                                 OnMouseReleased = 2, OnNodeExpanded = 2, OnNodeSelected = 2, OnTabSelected = 2,
                                 OnTextChanged = 2, Page = 2, Parent = 0, Picture = 2,
                                 RootNode = 0, RowSource = 2, TabIndex = 2, Text = 0, TipText = 2,
                                 TripleState = 2, URL = 2, Value = 3, Visible = 2, Width = 2,
                                 X = 2, Y = 2, XControlModel = 0, XControlView = 0,
                                 XGridColumnModel = 0, XGridDataModel = 0, XTreeDataModel = 0)

        # Root or node related properties do not start with X and, nevertheless, return a UNO object
        @property
        def CurrentNode(self):
            return self.EXEC(self.objectreference, self.vbGet + self.flgUno, 'CurrentNode')

        @property
        def RootNode(self):
            return self.EXEC(self.objectreference, self.vbGet + self.flgUno, 'RootNode')

        def AddSubNode(self, parentnode, displayvalue, datavalue = ScriptForge.cstSymEmpty):
            return self.ExecMethod(self.vbMethod + self.flgUno, 'AddSubNode', parentnode, displayvalue, datavalue)

        def AddSubTree(self, parentnode, flattree, withdatavalue = False):
            return self.ExecMethod(self.vbMethod, 'AddSubTree', parentnode, flattree, withdatavalue)

        def CreateRoot(self, displayvalue, datavalue = ScriptForge.cstSymEmpty):
            return self.ExecMethod(self.vbMethod + self.flgUno, 'CreateRoot', displayvalue, datavalue)

        def FindNode(self, displayvalue, datavalue = ScriptForge.cstSymEmpty, casesensitive = False):
            return self.ExecMethod(self.vbMethod + self.flgUno, 'FindNode', displayvalue, datavalue, casesensitive)

        def Resize(self, left = -99999, top = -99999, width = -1, height = -1):
            return self.ExecMethod(self.vbMethod, 'Resize', left, top, width, height)

        def SetFocus(self):
            return self.ExecMethod(self.vbMethod, 'SetFocus')

        def SetTableData(self, dataarray, widths = (1,), alignments = '', rowheaderwidth = 10):
            return self.ExecMethod(self.vbMethod + self.flgArrayArg, 'SetTableData', dataarray, widths, alignments,
                                   rowheaderwidth)

        def WriteLine(self, line = ''):
            return self.ExecMethod(self.vbMethod, 'WriteLine', line)


# #####################################################################################################################
#                       SFDocuments CLASS    (alias of SFDocuments Basic library)                                   ###
# #####################################################################################################################
class SFDocuments:
    """
        The SFDocuments class gathers a number of classes, methods and properties making easy
        managing and manipulating LibreOffice documents
        """
    pass

    # #########################################################################
    # SF_Document CLASS
    # #########################################################################
    class SF_Document(SFServices):
        """
            The methods and properties are generic for all types of documents: they are combined in the
            current SF_Document class
                - saving, closing documents
                - accessing their standard or custom properties
            Specific properties and methods are implemented in the concerned subclass(es) SF_Calc, SF_Base, ...
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFDocuments.Document'
        servicesynonyms = ('document', 'sfdocuments.document')
        serviceproperties = dict(CustomProperties = 3, Description = 3, DocumentProperties = 1,
                                 DocumentType = 0, ExportFilters = 0, FileSystem = 0, ImportFilters = 0,
                                 IsAlive = 1, IsBase = 0, IsCalc = 0, IsDraw = 0, IsFormDocument = 0,
                                 IsImpress = 0, IsMath = 0, IsWriter = 0, Keywords = 3, MenuHeaders = 1,
                                 Readonly = 1, StyleFamilies = 1, Subject = 3, Title = 3, XComponent = 0,
                                 XDocumentSettings = 0)

        @classmethod
        def ReviewServiceArgs(cls, windowname = ''):
            """
                Transform positional and keyword arguments into positional only
                """
            return windowname,

        def Activate(self):
            return self.ExecMethod(self.vbMethod, 'Activate')

        def CloseDocument(self, saveask = True):
            return self.ExecMethod(self.vbMethod, 'CloseDocument', saveask)

        def ContextMenus(self, contextmenuname = '', submenuchar = '>'):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'ContextMenus', contextmenuname, submenuchar)

        def CreateMenu(self, menuheader, before = '', submenuchar = '>'):
            return self.ExecMethod(self.vbMethod, 'CreateMenu', menuheader, before, submenuchar)

        def DeleteStyles(self, family, styleslist):
            # Exclude Base, FormDocument and Math
            doctype = self.DocumentType
            if doctype in ('Calc', 'Writer', 'Draw', 'Impress'):
                return self.ExecMethod(self.vbMethod, 'DeleteStyles', family, styleslist)
            raise RuntimeError('The \'DeleteStyles\' method is not applicable to {0} documents'.format(doctype))

        def Echo(self, echoon = True, hourglass = False):
            return self.ExecMethod(self.vbMethod, 'Echo', echoon, hourglass)

        def ExportAsPDF(self, filename, overwrite = False, pages = '', password = '', watermark = ''):
            return self.ExecMethod(self.vbMethod, 'ExportAsPDF', filename, overwrite, pages, password, watermark)

        def PrintOut(self, pages = '', copies = 1):
            return self.ExecMethod(self.vbMethod, 'PrintOut', pages, copies)

        def RemoveMenu(self, menuheader):
            return self.ExecMethod(self.vbMethod, 'RemoveMenu', menuheader)

        def RunCommand(self, command, *args, **kwargs):
            params = tuple([command] + list(args) + ScriptForge.unpack_args(kwargs))
            return self.ExecMethod(self.vbMethod, 'RunCommand', *params)

        def Save(self):
            return self.ExecMethod(self.vbMethod, 'Save')

        def SaveAs(self, filename, overwrite = False, password = '', filtername = '', filteroptions = ''):
            return self.ExecMethod(self.vbMethod, 'SaveAs', filename, overwrite, password, filtername, filteroptions)

        def SaveCopyAs(self, filename, overwrite = False, password = '', filtername = '', filteroptions = ''):
            return self.ExecMethod(self.vbMethod, 'SaveCopyAs', filename, overwrite,
                                   password, filtername, filteroptions)

        def SetPrinter(self, printer = '', orientation = '', paperformat = ''):
            return self.ExecMethod(self.vbMethod, 'SetPrinter', printer, orientation, paperformat)

        def Styles(self, family, namepattern = '', used = ScriptForge.cstSymEmpty,
                   userdefined = ScriptForge.cstSymEmpty, parentstyle = '', category = ''):
            # Exclude Base and Math
            doctype = self.DocumentType
            if doctype in ('Calc', 'Writer', 'FormDocument', 'Draw', 'Impress'):
                return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'Styles', family, namepattern, used,
                                       userdefined, parentstyle, category)
            raise RuntimeError('The \'Styles\' method is not applicable to {0} documents'.format(doctype))

        def Toolbars(self, toolbarname = ''):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'Toolbars', toolbarname)

        def XStyle(self, family, stylename):
            # Exclude Base and Math
            doctype = self.DocumentType
            if doctype in ('Calc', 'Writer', 'FormDocument', 'Draw', 'Impress'):
                # XStyles() DOES NOT WORK in bridged mode ?!? Works normally in direct mode.
                if ScriptForge.remoteprocess:
                    return None
                return self.ExecMethod(self.vbMethod + self.flgUno, 'XStyle', family, stylename)
            raise RuntimeError('The \'XStyle\' method is not applicable to {0} documents'.format(doctype))

    # #########################################################################
    # SF_Base CLASS
    # #########################################################################
    class SF_Base(SF_Document, SFServices):
        """
            The SF_Base module is provided mainly to block parent properties that are NOT applicable to Base documents
            In addition, it provides methods to identify form documents and access their internal forms
            (read more elsewhere (the "SFDocuments.Form" service) about this subject)
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFDocuments.Base'
        servicesynonyms = ('base', 'scriptforge.base')
        serviceproperties = dict(DocumentType = 0, FileSystem = 0, IsAlive = 1, IsBase = 0, IsCalc = 0,
                                 IsDraw = 0, IsFormDocument = 0, IsImpress = 0, IsMath = 0,
                                 IsWriter = 0, MenuHeaders = 1, XComponent = 0)

        @classmethod
        def ReviewServiceArgs(cls, windowname = ''):
            """
                Transform positional and keyword arguments into positional only
                """
            return windowname,

        def CloseDocument(self, saveask = True):
            return self.ExecMethod(self.vbMethod, 'CloseDocument', saveask)

        def CloseFormDocument(self, formdocument):
            return self.ExecMethod(self.vbMethod, 'CloseFormDocument', formdocument)

        def FormDocuments(self):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'FormDocuments')

        def Forms(self, formdocument, form = ''):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'Forms', formdocument, form)

        def GetDatabase(self, user = '', password = ''):
            return self.ExecMethod(self.vbMethod, 'GetDatabase', user, password)

        def IsLoaded(self, formdocument):
            return self.ExecMethod(self.vbMethod, 'IsLoaded', formdocument)

        def OpenFormDocument(self, formdocument, designmode = False):
            return self.ExecMethod(self.vbMethod, 'OpenFormDocument', formdocument, designmode)

        def OpenQuery(self, queryname, designmode = False):
            return self.ExecMethod(self.vbMethod, 'OpenQuery', queryname, designmode)

        def OpenTable(self, tablename, designmode = False):
            return self.ExecMethod(self.vbMethod, 'OpenTable', tablename, designmode)

        def PrintOut(self, formdocument, pages = '', copies = 1):
            return self.ExecMethod(self.vbMethod, 'PrintOut', formdocument, pages, copies)

        def SetPrinter(self, formdocument = '', printer = '', orientation = '', paperformat = ''):
            return self.ExecMethod(self.vbMethod, 'SetPrinter', formdocument, printer, orientation, paperformat)

    # #########################################################################
    # SF_Calc CLASS
    # #########################################################################
    class SF_Calc(SF_Document, SFServices):
        """
            The SF_Calc module is focused on :
                - management (copy, insert, move, ...) of sheets within a Calc document
                - exchange of data between Basic data structures and Calc ranges of values
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFDocuments.Calc'
        servicesynonyms = ('calc', 'sfdocuments.calc')
        serviceproperties = dict(CurrentSelection = 3, CustomProperties = 3, Description = 3,
                                 DocumentProperties = 1, DocumentType = 0, ExportFilters = 0,
                                 FileSystem = 0, ImportFilters = 0, IsAlive = 1, IsBase = 0, IsCalc = 0,
                                 IsDraw = 0, IsFormDocument = 0, IsImpress = 0, IsMath = 0,
                                 IsWriter = 0, Keywords = 3, MenuHeaders = 1, Readonly = 1, Sheets = 1,
                                 StyleFamilies = 0, Subject = 3, Title = 3, XComponent = 0,
                                 XDocumentSettings = 0)

        @classmethod
        def ReviewServiceArgs(cls, windowname = ''):
            """
                Transform positional and keyword arguments into positional only
                """
            return windowname,

        # Next functions are implemented in Basic as read-only properties with 1 argument
        def FirstCell(self, rangename):
            return self.GetProperty('FirstCell', rangename)

        def FirstColumn(self, rangename):
            return self.GetProperty('FirstColumn', rangename)

        def FirstRow(self, rangename):
            return self.GetProperty('FirstRow', rangename)

        def Height(self, rangename):
            return self.GetProperty('Height', rangename)

        def LastCell(self, rangename):
            return self.GetProperty('LastCell', rangename)

        def LastColumn(self, rangename):
            return self.GetProperty('LastColumn', rangename)

        def LastRow(self, rangename):
            return self.GetProperty('LastRow', rangename)

        def Range(self, rangename):
            return self.GetProperty('Range', rangename)

        def Region(self, rangename):
            return self.GetProperty('Region', rangename)

        def Sheet(self, sheetname):
            return self.GetProperty('Sheet', sheetname)

        def SheetName(self, rangename):
            return self.GetProperty('SheetName', rangename)

        def Width(self, rangename):
            return self.GetProperty('Width', rangename)

        def XCellRange(self, rangename):
            return self.ExecMethod(self.vbGet + self.flgUno, 'XCellRange', rangename)

        def XRectangle(self, rangename):
            return self.ExecMethod(self.vbGet + self.flgUno, 'XRectangle', rangename)

        def XSheetCellCursor(self, rangename):
            return self.ExecMethod(self.vbGet + self.flgUno, 'XSheetCellCursor', rangename)

        def XSpreadsheet(self, sheetname):
            return self.ExecMethod(self.vbGet + self.flgUno, 'XSpreadsheet', sheetname)

        # Usual methods
        def A1Style(self, row1, column1, row2 = 0, column2 = 0, sheetname = ''):
            return self.ExecMethod(self.vbMethod, 'A1Style', row1, column1, row2, column2, sheetname)

        def Activate(self, sheetname = ''):
            return self.ExecMethod(self.vbMethod, 'Activate', sheetname)

        def AlignRange(self, targetrange, alignment, filterformula = '', filterscope = ''):
            return self.ExecMethod(self.vbMethod, 'AlignRange', targetrange, alignment, filterformula, filterscope)

        def BorderRange(self, targetrange, borders, filterformula = '', filterscope = ''):
            return self.ExecMethod(self.vbMethod, 'BorderRange', targetrange, borders, filterformula, filterscope)

        def Charts(self, sheetname, chartname = ''):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'Charts', sheetname, chartname)

        def ClearAll(self, range, filterformula = '', filterscope = ''):
            return self.ExecMethod(self.vbMethod, 'ClearAll', range, filterformula, filterscope)

        def ClearFormats(self, range, filterformula = '', filterscope = ''):
            return self.ExecMethod(self.vbMethod, 'ClearFormats', range, filterformula, filterscope)

        def ClearValues(self, range, filterformula = '', filterscope = ''):
            return self.ExecMethod(self.vbMethod, 'ClearValues', range, filterformula, filterscope)

        def ColorizeRange(self, targetrange, foreground = -1, background = -1, filterformula = '', filterscope = ''):
            return self.ExecMethod(self.vbMethod, 'ColorizeRange', targetrange, foreground, background,
                                   filterformula, filterscope)

        def CompactLeft(self, range, wholecolumn = False, filterformula = ''):
            return self.ExecMethod(self.vbMethod, 'CompactLeft', range, wholecolumn, filterformula)

        def CompactUp(self, range, wholerow = False, filterformula = ''):
            return self.ExecMethod(self.vbMethod, 'CompactUp', range, wholerow, filterformula)

        def CopySheet(self, sheetname, newname, beforesheet = 32768):
            sheet = (sheetname.objectreference if isinstance(sheetname, SFDocuments.SF_CalcReference) else sheetname)
            return self.ExecMethod(self.vbMethod + self.flgObject, 'CopySheet', sheet, newname, beforesheet)

        def CopySheetFromFile(self, filename, sheetname, newname, beforesheet = 32768):
            sheet = (sheetname.objectreference if isinstance(sheetname, SFDocuments.SF_CalcReference) else sheetname)
            return self.ExecMethod(self.vbMethod + self.flgObject, 'CopySheetFromFile',
                                   filename, sheet, newname, beforesheet)

        def CopyToCell(self, sourcerange, destinationcell):
            range = (sourcerange.objectreference if isinstance(sourcerange, SFDocuments.SF_CalcReference)
                     else sourcerange)
            return self.ExecMethod(self.vbMethod + self.flgObject, 'CopyToCell', range, destinationcell)

        def CopyToRange(self, sourcerange, destinationrange):
            range = (sourcerange.objectreference if isinstance(sourcerange, SFDocuments.SF_CalcReference)
                     else sourcerange)
            return self.ExecMethod(self.vbMethod + self.flgObject, 'CopyToRange', range, destinationrange)

        def CreateChart(self, chartname, sheetname, range, columnheader = False, rowheader = False):
            return self.ExecMethod(self.vbMethod, 'CreateChart', chartname, sheetname, range, columnheader, rowheader)

        def CreatePivotTable(self, pivottablename, sourcerange, targetcell, datafields = ScriptForge.cstSymEmpty,
                             rowfields = ScriptForge.cstSymEmpty, columnfields = ScriptForge.cstSymEmpty,
                             filterbutton = True, rowtotals = True, columntotals = True):
            return self.ExecMethod(self.vbMethod, 'CreatePivotTable', pivottablename, sourcerange, targetcell,
                                   datafields, rowfields, columnfields, filterbutton, rowtotals, columntotals)

        def DAvg(self, range):
            return self.ExecMethod(self.vbMethod, 'DAvg', range)

        def DCount(self, range):
            return self.ExecMethod(self.vbMethod, 'DCount', range)

        def DMax(self, range):
            return self.ExecMethod(self.vbMethod, 'DMax', range)

        def DMin(self, range):
            return self.ExecMethod(self.vbMethod, 'DMin', range)

        def DSum(self, range):
            return self.ExecMethod(self.vbMethod, 'DSum', range)

        def DecorateFont(self, targetrange, fontname = '', fontsize = 0, decoration = '',
                         filterformula = '', filterscope = ''):
            return self.ExecMethod(self.vbMethod, 'DecorateFont', targetrange, fontname, fontsize, decoration,
                                   filterformula, filterscope)

        def ExportRangeToFile(self, range, filename, imagetype = 'pdf', overwrite = False):
            return self.ExecMethod(self.vbMethod, 'ExportRangeToFile', range, filename, imagetype, overwrite)

        def FormatRange(self, targetrange, numberformat, locale = '', filterformula = '', filterscope = ''):
            return self.ExecMethod(self.vbMethod, 'FormatRange', targetrange, numberformat, locale,
                                   filterformula, filterscope)

        def Forms(self, sheetname, form = ''):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'Forms', sheetname, form)

        def GetColumnName(self, columnnumber):
            return self.ExecMethod(self.vbMethod, 'GetColumnName', columnnumber)

        def GetFormula(self, range):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'GetFormula', range)

        def GetValue(self, range):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'GetValue', range)

        def ImportFromCSVFile(self, filename, destinationcell, filteroptions = ScriptForge.cstSymEmpty):
            return self.ExecMethod(self.vbMethod, 'ImportFromCSVFile', filename, destinationcell, filteroptions)

        def ImportFromDatabase(self, filename = '', registrationname = '', destinationcell = '', sqlcommand = '',
                               directsql = False):
            return self.ExecMethod(self.vbMethod, 'ImportFromDatabase', filename, registrationname,
                                   destinationcell, sqlcommand, directsql)

        def ImportStylesFromFile(self, filename = '', families = '', overwrite = False):
            return self.ExecMethod(self.vbMethod, 'ImportStylesFromFile', filename, families, overwrite)

        def InsertSheet(self, sheetname, beforesheet = 32768):
            return self.ExecMethod(self.vbMethod, 'InsertSheet', sheetname, beforesheet)

        def Intersect(self, range1, range2):
            return self.ExecMethod(self.vbMethod, 'Intersect', range1, range2)

        def MoveRange(self, source, destination):
            return self.ExecMethod(self.vbMethod, 'MoveRange', source, destination)

        def MoveSheet(self, sheetname, beforesheet = 32768):
            return self.ExecMethod(self.vbMethod, 'MoveSheet', sheetname, beforesheet)

        def Offset(self, range, rows = 0, columns = 0, height = ScriptForge.cstSymEmpty,
                   width = ScriptForge.cstSymEmpty):
            return self.ExecMethod(self.vbMethod, 'Offset', range, rows, columns, height, width)

        def OpenRangeSelector(self, title = '', selection = '~', singlecell = False, closeafterselect = True):
            return self.ExecMethod(self.vbMethod, 'OpenRangeSelector', title, selection, singlecell, closeafterselect)

        def Printf(self, inputstr, range, tokencharacter = '%'):
            return self.ExecMethod(self.vbMethod, 'Printf', inputstr, range, tokencharacter)

        def PrintOut(self, sheetname = '~', pages = '', copies = 1):
            return self.ExecMethod(self.vbMethod, 'PrintOut', sheetname, pages, copies)

        def RemoveDuplicates(self, range, columns = 1, header = False, casesensitive = False, mode = 'COMPACT'):
            return self.ExecMethod(self.vbMethod, 'RemoveDuplicates', range, columns, header, casesensitive, mode)

        def RemoveSheet(self, sheetname):
            return self.ExecMethod(self.vbMethod, 'RemoveSheet', sheetname)

        def RenameSheet(self, sheetname, newname):
            return self.ExecMethod(self.vbMethod, 'RenameSheet', sheetname, newname)

        def SetArray(self, targetcell, value):
            return self.ExecMethod(self.vbMethod + self.flgArrayArg, 'SetArray', targetcell, value)

        def SetCellStyle(self, targetrange, style, filterformula = '', filterscope = ''):
            return self.ExecMethod(self.vbMethod, 'SetCellStyle', targetrange, style, filterformula, filterscope)

        def SetFormula(self, targetrange, formula):
            return self.ExecMethod(self.vbMethod + self.flgArrayArg, 'SetFormula', targetrange, formula)

        def SetValue(self, targetrange, value):
            return self.ExecMethod(self.vbMethod + self.flgArrayArg, 'SetValue', targetrange, value)

        def ShiftDown(self, range, wholerow = False, rows = 0):
            return self.ExecMethod(self.vbMethod, 'ShiftDown', range, wholerow, rows)

        def ShiftLeft(self, range, wholecolumn = False, columns = 0):
            return self.ExecMethod(self.vbMethod, 'ShiftLeft', range, wholecolumn, columns)

        def ShiftRight(self, range, wholecolumn = False, columns = 0):
            return self.ExecMethod(self.vbMethod, 'ShiftRight', range, wholecolumn, columns)

        def ShiftUp(self, range, wholerow = False, rows = 0):
            return self.ExecMethod(self.vbMethod, 'ShiftUp', range, wholerow, rows)

        def SortRange(self, range, sortkeys, sortorder = 'ASC', destinationcell = ScriptForge.cstSymEmpty,
                      containsheader = False, casesensitive = False, sortcolumns = False):
            return self.ExecMethod(self.vbMethod, 'SortRange', range, sortkeys, sortorder, destinationcell,
                                   containsheader, casesensitive, sortcolumns)

    # #########################################################################
    # SF_CalcReference CLASS
    # #########################################################################
    class SF_CalcReference(SFServices):
        """
            The SF_CalcReference class has as unique role to hold sheet and range references.
            They are implemented in Basic as Type ... End Type data structures
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFDocuments.CalcReference'
        servicesynonyms = ()
        serviceproperties = dict()

    # #########################################################################
    # SF_Chart CLASS
    # #########################################################################
    class SF_Chart(SFServices):
        """
            The SF_Chart module is focused on the description of chart documents
            stored in Calc sheets.
            With this service, many chart types and chart characteristics available
            in the user interface can be read or modified.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFDocuments.Chart'
        servicesynonyms = ()
        serviceproperties = dict(ChartType = 2, Deep = 2, Dim3D = 2, Exploded = 2, Filled = 2,
                                 Legend = 2, Percent = 2, Stacked = 2, Title = 2,
                                 XChartObj = 0, XDiagram = 0, XShape = 0, XTableChart = 0,
                                 XTitle = 2, YTitle = 2)

        def ExportToFile(self, filename, imagetype = 'png', overwrite = False):
            return self.ExecMethod(self.vbMethod, 'ExportToFile', filename, imagetype, overwrite)

        def Resize(self, xpos = -1, ypos = -1, width = -1, height = -1):
            return self.ExecMethod(self.vbMethod, 'Resize', xpos, ypos, width, height)

    # #########################################################################
    # SF_Form CLASS
    # #########################################################################
    class SF_Form(SFServices):
        """
            Management of forms defined in SnipeOffice documents. Supported types are Base, Calc and Writer documents.
            It includes the management of subforms
            Each instance of the current class represents a single form or a single subform
            A form may optionally be (understand "is often") linked to a data source manageable with
            the SFDatabases.Database service. The current service offers rapid access to that service.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFDocuments.Form'
        servicesynonyms = ()
        serviceproperties = dict(AllowDeletes = 2, AllowInserts = 2, AllowUpdates = 2, BaseForm = 0,
                                 Bookmark = 3, CurrentRecord = 3, Filter = 3, LinkChildFields = 0,
                                 LinkParentFields = 0, Name = 0,
                                 OnApproveCursorMove = 2, OnApproveParameter = 2, OnApproveReset = 2,
                                 OnApproveRowChange = 2, OnApproveSubmit = 2, OnConfirmDelete = 2,
                                 OnCursorMoved = 2, OnErrorOccurred = 2, OnLoaded = 2, OnReloaded = 2,
                                 OnReloading = 2, OnResetted = 2, OnRowChanged = 2, OnUnloaded = 2,
                                 OnUnloading = 2, OrderBy = 3, Parent = 0, RecordSource = 2, XForm = 0)

        def Activate(self):
            return self.ExecMethod(self.vbMethod, 'Activate')

        def CloseFormDocument(self):
            return self.ExecMethod(self.vbMethod, 'CloseFormDocument')

        def Controls(self, controlname = ''):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'Controls', controlname)

        def GetDatabase(self, user = '', password = ''):
            return self.ExecMethod(self.vbMethod, 'GetDatabase', user, password)

        def MoveFirst(self):
            return self.ExecMethod(self.vbMethod, 'MoveFirst')

        def MoveLast(self):
            return self.ExecMethod(self.vbMethod, 'MoveLast')

        def MoveNew(self):
            return self.ExecMethod(self.vbMethod, 'MoveNew')

        def MoveNext(self, offset = 1):
            return self.ExecMethod(self.vbMethod, 'MoveNext', offset)

        def MovePrevious(self, offset = 1):
            return self.ExecMethod(self.vbMethod, 'MovePrevious', offset)

        def Requery(self):
            return self.ExecMethod(self.vbMethod, 'Requery')

        def Subforms(self, subform = ''):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'Subforms', subform)

    # #########################################################################
    # SF_FormControl CLASS
    # #########################################################################
    class SF_FormControl(SFServices):
        """
            Manage the controls belonging to a form or subform stored in a document.
            Each instance of the current class represents a single control within a form, a subform or a tablecontrol.
            A prerequisite is that all controls within the same form, subform or tablecontrol must have
            a unique name.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFDocuments.FormControl'
        servicesynonyms = ()
        serviceproperties = dict(Action = 2, Caption = 2, ControlSource = 0, ControlType = 0,
                                 Default = 2, DefaultValue = 2, Enabled = 2, Format = 2,
                                 ListCount = 0, ListIndex = 3, ListSource = 2, ListSourceType = 2,
                                 Locked = 2, MultiSelect = 2, Name = 0,
                                 OnActionPerformed = 2, OnAdjustmentValueChanged = 2,
                                 OnApproveAction = 2, OnApproveReset = 2, OnApproveUpdate = 2,
                                 OnChanged = 2, OnErrorOccurred = 2, OnFocusGained = 2, OnFocusLost = 2,
                                 OnItemStateChanged = 2, OnKeyPressed = 2, OnKeyReleased = 2,
                                 OnMouseDragged = 2, OnMouseEntered = 2, OnMouseExited = 2,
                                 OnMouseMoved = 2, OnMousePressed = 2, OnMouseReleased = 2, OnResetted = 2,
                                 OnTextChanged = 2, OnUpdated = 2, Parent = 0, Picture = 2,
                                 Required = 2, Text = 0, TipText = 2, TripleState = 2, Value = 3,
                                 Visible = 2, XControlModel = 0, XControlView = 0)

        def Controls(self, controlname = ''):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'Controls', controlname)

        def SetFocus(self):
            return self.ExecMethod(self.vbMethod, 'SetFocus')

    # #########################################################################
    # SF_FormDocument CLASS
    # #########################################################################
    class SF_FormDocument(SF_Document, SFServices):
        """
            The orchestration of Base form documents (aka Base Forms, but this is confusing)
            and the identification of and the access to their controls.
            Form documents are always contained in a Base document.
            They should not be confused with Writer documents containing forms,
            even if it is easy to convert the former to the latter.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFDocuments.FormDocument'
        servicesynonyms = ('formdocument', 'sfdocuments.formdocument')
        serviceproperties = dict(DocumentType = 0, FileSystem = 0, IsAlive = 1, IsBase = 0, IsCalc = 0,
                                 IsDraw = 0, IsFormDocument = 0, IsImpress = 0, IsMath = 0,
                                 IsWriter = 0, MenuHeaders = 1, Readonly = 0, StyleFamilies = 0, XComponent = 0,
                                 XDocumentSettings = 0)

        @classmethod
        def ReviewServiceArgs(cls, windowname = ''):
            """
                Transform positional and keyword arguments into positional only
                """
            return windowname,

        def CloseDocument(self):
            return self.ExecMethod(self.vbMethod, 'CloseDocument')

        def Forms(self, form = ''):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'Forms', form)

        def GetDatabase(self, user = '', password = ''):
            return self.ExecMethod(self.vbMethod, 'GetDatabase', user, password)

        def PrintOut(self, pages = '', copies = 1, printbackground = True, printblankpages = False,
                     printevenpages = True, printoddpages = True, printimages = True):
            return self.ExecMethod(self.vbMethod, 'PrintOut', pages, copies, printbackground, printblankpages,
                                   printevenpages, printoddpages, printimages)

    # #########################################################################
    # SF_Writer CLASS
    # #########################################################################
    class SF_Writer(SF_Document, SFServices):
        """
            The SF_Writer module is focused on :
                - TBD
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFDocuments.Writer'
        servicesynonyms = ('writer', 'sfdocuments.writer')
        serviceproperties = dict(CustomProperties = 3, Description = 3, DocumentProperties = 1,
                                 DocumentType = 0, ExportFilters = 0, FileSystem = 0, ImportFilters = 0,
                                 IsAlive = 1, IsBase = 0, IsCalc = 0, IsDraw = 0, IsFormDocument = 0,
                                 IsImpress = 0, IsMath = 0, IsWriter = 0, Keywords = 3, MenuHeaders = 1,
                                 Readonly = 1, StyleFamilies = 1, Subject = 3, Title = 3, XComponent = 0,
                                 XDocumentSettings = 0)

        @classmethod
        def ReviewServiceArgs(cls, windowname = ''):
            """
                Transform positional and keyword arguments into positional only
                """
            return windowname,

        def Forms(self, form = ''):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'Forms', form)

        def ImportStylesFromFile(self, filename = '', families = '', overwrite = False):
            return self.ExecMethod(self.vbMethod, 'ImportStylesFromFile', filename, families, overwrite)

        def PrintOut(self, pages = '', copies = 1, printbackground = True, printblankpages = False,
                     printevenpages = True, printoddpages = True, printimages = True):
            return self.ExecMethod(self.vbMethod, 'PrintOut', pages, copies, printbackground, printblankpages,
                                   printevenpages, printoddpages, printimages)


# #####################################################################################################################
#                       SFWidgets CLASS    (alias of SFWidgets Basic library)                                       ###
# #####################################################################################################################
class SFWidgets:
    """
        The SFWidgets class manages toolbars and popup menus
        """
    pass

    # #########################################################################
    # SF_Menu CLASS
    # #########################################################################
    class SF_Menu(SFServices):
        """
            Display a menu in the menubar of a document or a form document.
            After use, the menu will not be saved neither in the application settings, nor in the document.
            The menu will be displayed, as usual, when its header in the menubar is clicked.
            When one of its items is selected, there are 3 alternative options:
            - a UNO command (like ".uno:About") is triggered
            - a user script is run receiving a standard argument defined in this service
            - one of above combined with a toggle of the status of the item
            The menu is described from top to bottom. Each menu item receives a numeric and a string identifier.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFWidgets.Menu'
        servicesynonyms = ('menu', 'sfwidgets.menu')
        serviceproperties = dict(ShortcutCharacter = 0, SubmenuCharacter = 0)

        def AddCheckBox(self, menuitem, name = '', status = False, icon = '', tooltip = '',
                        command = '', script = ''):
            return self.ExecMethod(self.vbMethod, 'AddCheckBox', menuitem, name, status, icon, tooltip,
                                   command, script)

        def AddItem(self, menuitem, name = '', icon = '', tooltip = '', command = '', script = ''):
            return self.ExecMethod(self.vbMethod, 'AddItem', menuitem, name, icon, tooltip, command, script)

        def AddRadioButton(self, menuitem, name = '', status = False, icon = '', tooltip = '',
                           command = '', script = ''):
            return self.ExecMethod(self.vbMethod, 'AddRadioButton', menuitem, name, status, icon, tooltip,
                                   command, script)

    # #########################################################################
    # SF_ContextMenu CLASS
    # #########################################################################
    class SF_ContextMenu(SFServices):
        """
            A context menu is obtained by a right-click on several areas of a document.
            Each component model has its own set of context menus.

            A context menu is usually predefined at LibreOffice installation.
            Customization is done statically with the Tools + Customize dialog.

            The actual service provides means
                - to make temporary additions at the bottom of a context menu,
                - to replace entirely a context menu.
            Those changes are lost when the document is closed.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFWidgets.ContextMenu'
        servicesynonyms = ('contextmenu', 'sfwidgets.contextmenu')
        serviceproperties = dict(ParentDocument = 0, ShortcutCharacter = 0, SubmenuCharacter = 0)

        def Activate(self, enable = True):
            return self.ExecMethod(self.vbMethod, 'Activate', enable)

        def AddItem(self, menuitem, command = '', script = ''):
            return self.ExecMethod(self.vbMethod, 'AddItem', menuitem, command, script)

        def RemoveAllItems(self):
            return self.ExecMethod(self.vbMethod, 'RemoveAllItems')


    # #########################################################################
    # SF_PopupMenu CLASS
    # #########################################################################
    class SF_PopupMenu(SFServices):
        """
            Display a popup menu anywhere and any time.
            A popup menu is usually triggered by a mouse action (typically a right-click) on a dialog, a form
            or one of their controls. In this case the menu will be displayed below the clicked area.
            When triggered by other events, including in the normal flow of a user script, the script should
            provide the coordinates of the topleft edge of the menu versus the actual component.
            The menu is described from top to bottom. Each menu item receives a numeric and a string identifier.
            The execute() method returns the item selected by the user.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFWidgets.PopupMenu'
        servicesynonyms = ('popupmenu', 'sfwidgets.popupmenu')
        serviceproperties = dict(ShortcutCharacter = 0, SubmenuCharacter = 0)

        @classmethod
        def ReviewServiceArgs(cls, event = None, x = 0, y = 0, submenuchar = ''):
            """
                Transform positional and keyword arguments into positional only
                """
            return event, x, y, submenuchar

        def AddCheckBox(self, menuitem, name = '', status = False, icon = '', tooltip = ''):
            return self.ExecMethod(self.vbMethod, 'AddCheckBox', menuitem, name, status, icon, tooltip)

        def AddItem(self, menuitem, name = '', icon = '', tooltip = ''):
            return self.ExecMethod(self.vbMethod, 'AddItem', menuitem, name, icon, tooltip)

        def AddRadioButton(self, menuitem, name = '', status = False, icon = '', tooltip = ''):
            return self.ExecMethod(self.vbMethod, 'AddRadioButton', menuitem, name, status, icon, tooltip)

        def Execute(self, returnid = True):
            return self.ExecMethod(self.vbMethod, 'Execute', returnid)

    # #########################################################################
    # SF_Toolbar CLASS
    # #########################################################################
    class SF_Toolbar(SFServices):
        """
            Each component has its own set of toolbars, depending on the component type
            (Calc, Writer, Basic IDE, ...).
            In the context of the actual class, a toolbar is presumed defined statically:
                - either by the application
                - or by a customization done by the user.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFWidgets.Toolbar'
        servicesynonyms = ('toolbar', 'sfwidgets.toolbar')
        serviceproperties = dict(BuiltIn = 0, Docked = 1, HasGlobalScope = 0, Name = 0,
                                 ResourceURL = 0, Visible = 3, XUIElement = 0)

        def ToolbarButtons(self, buttonname = ''):
            return self.ExecMethod(self.vbMethod + self.flgArrayRet, 'ToolbarButtons', buttonname)

    # #########################################################################
    # SF_ToolbarButton CLASS
    # #########################################################################
    class SF_ToolbarButton(SFServices):
        """
            A toolbar consists in a series of graphical controls to trigger actions.
            The "Toolbar" service gives access to the "ToolbarButton" service to manage
            the individual buttons belonging to the toolbar.
            """
        # Mandatory class properties for service registration
        serviceimplementation = 'basic'
        servicename = 'SFWidgets.ToolbarButton'
        servicesynonyms = ('toolbarbutton', 'sfwidgets.toolbarbutton')
        serviceproperties = dict(Caption = 0, Height = 0, Index = 0, OnClick = 2, Parent = 0,
                                 TipText = 2, Visible = 2, Width = 0, X = 0, Y = 0)

        def Execute(self):
            return self.ExecMethod(self.vbMethod, 'Execute')


# ##############################################False##################################################################
#                           CreateScriptService()                                                                   ###
# #####################################################################################################################
def CreateScriptService(service, *args, **kwargs):
    """
        A service being the name of a collection of properties and methods,
        this method returns either
            - the Python object mirror of the Basic object implementing the requested service
            - the Python object implementing the service itself

        A service may be designated by its official name, stored in its class.servicename
        or by one of its synonyms stored in its class.servicesynonyms list
        If the service is not identified, the service creation is delegated to Basic, that might raise an error
        if still not identified there

        :param service: the name of the service as a string 'library.service' - cased exactly
                or one of its synonyms
        :param args: the arguments to pass to the service constructor
        :return: the service as a Python object
        """
    # Init at each CreateScriptService() invocation
    #       CreateScriptService is usually the first statement in user scripts requesting ScriptForge services
    #       ScriptForge() is optional in user scripts when Python process inside LibreOffice process
    if ScriptForge.SCRIPTFORGEINITDONE is False:
        ScriptForge()

    def ResolveSynonyms(servicename):
        """
            Synonyms within service names implemented in Python or predefined are resolved here
            :param servicename: The short name of the service
            :return: The official service name if found, the argument otherwise
            """
        for cls in SFServices.__subclasses__():
            if servicename.lower() in cls.servicesynonyms:
                return cls.servicename
        return servicename

    #
    # Check the list of available services
    scriptservice = ResolveSynonyms(service)
    if scriptservice in ScriptForge.serviceslist:
        serv = ScriptForge.serviceslist[scriptservice]
        # Check if the requested service is within the Python world
        if serv.serviceimplementation == 'python':
            return serv(*args)
        # Check if the service is a predefined standard Basic service
        elif scriptservice in ScriptForge.servicesmodules:
            return serv(ScriptForge.servicesmodules[scriptservice], classmodule = SFServices.moduleStandard)
    else:
        serv = None
    # The requested service is to be found in the Basic world
    # Check if the service must review the arguments
    if serv is not None:
        if hasattr(serv, 'ReviewServiceArgs'):
            # ReviewServiceArgs() must be a class method
            args = serv.ReviewServiceArgs(*args, **kwargs)
    # Get the service object back from Basic
    if len(args) == 0:
        serv = ScriptForge.InvokeBasicService('SF_Services', SFServices.vbMethod, 'CreateScriptService', service)
    else:
        serv = ScriptForge.InvokeBasicService('SF_Services', SFServices.vbMethod, 'CreateScriptService',
                                              service, *args)
    return serv


createScriptService, createscriptservice = CreateScriptService, CreateScriptService


# ###############################################################################
# FOR TYPING HINTS, NEXT VARIABLE TYPES MAY BE IMPORTED IN USER SCRIPTS
# EXAMPLE:
#       from scriptforge import CALC, RANGE
#       def userfct(c: CALC, r: RANGE) -> RANGE:
#           r1: RANGE = "A1:K10"
# ###############################################################################
# List the available service types
#   SFScriptForge
ARRAY = SFScriptForge.SF_Array
BASIC = SFScriptForge.SF_Basic
DICTIONARY = SFScriptForge.SF_Dictionary
EXCEPTION = SFScriptForge.SF_Exception
FILESYSTEM = SFScriptForge.SF_FileSystem
L10N = SFScriptForge.SF_L10N
PLATFORM = SFScriptForge.SF_Platform
REGION = SFScriptForge.SF_Region
SESSION = SFScriptForge.SF_Session
STRING = SFScriptForge.SF_String
TEXTSTREAM = SFScriptForge.SF_TextStream
TIMER = SFScriptForge.SF_Timer
UI = SFScriptForge.SF_UI
#   SFDatabases
DATABASE = SFDatabases.SF_Database
DATASET = SFDatabases.SF_Dataset
DATASHEET = SFDatabases.SF_Datasheet
#   SFDialogs
DIALOG = SFDialogs.SF_Dialog
DIALOGCONTROL = SFDialogs.SF_DialogControl
#   SFDocuments
DOCUMENT = SFDocuments.SF_Document
BASE = SFDocuments.SF_Base
CALC = SFDocuments.SF_Calc
CALCREFERENCE = SFDocuments.SF_CalcReference
CHART = SFDocuments.SF_Chart
FORM = SFDocuments.SF_Form
FORMCONTROL = SFDocuments.SF_FormControl
FORMDOCUMENT = SFDocuments.SF_FormDocument
WRITER = SFDocuments.SF_Writer
#   SFWidgets
MENU = SFWidgets.SF_Menu
CONTEXTMENU = SFWidgets.SF_ContextMenu
POPUPMENU = SFWidgets.SF_PopupMenu
TOOLBAR = SFWidgets.SF_Toolbar
TOOLBARBUTTON = SFWidgets.SF_ToolbarButton
#   UNO
UNO = TypeVar('UNO')
#   Other
FILE = TypeVar('FILE', str, str)
SHEETNAME = TypeVar('SHEETNAME', str, str)
RANGE = TypeVar('RANGE', str, str)
SCRIPT_URI = TypeVar('SCRIPT_URI', str, str)
SQL_SELECT = TypeVar('SQL_SELECT', str, str)
SQL_ACTION = TypeVar('SQL_ACTION', str, str)


# ######################################################################
# Lists the scripts, that shall be visible inside the Basic/Python IDE
# ######################################################################

g_exportedScripts = ()
