/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

JUCEApplicationBase::CreateInstanceFunction JUCEApplicationBase::createInstance = nullptr;
JUCEApplicationBase* JUCEApplicationBase::appInstance = nullptr;

#if JUCE_IOS
void* JUCEApplicationBase::iOSCustomDelegate = nullptr;
#endif

void JUCEApplicationBase::setApplicationReturnValue (const int newReturnValue) noexcept
{
    appReturnValue = newReturnValue;
}

// This is called on the Mac and iOS where the OS doesn't allow the stack to unwind on shutdown..
void JUCEApplicationBase::appWillTerminateByForce()
{
    JUCE_AUTORELEASEPOOL
    {
        {
            const std::unique_ptr<JUCEApplicationBase> app (appInstance);

            if (app != nullptr)
                app->shutdownApp();
        }

        DeletedAtShutdown::deleteAll();
        MessageManager::deleteInstance();
    }
}

void JUCEApplicationBase::quit()
{
    MessageManager::getInstance()->stopDispatchLoop();
}

void JUCEApplicationBase::sendUnhandledException (const std::exception* const e,
                                                  const char* const sourceFile,
                                                  const int lineNumber)
{
    if (auto* app = JUCEApplicationBase::getInstance())
    {
        // If you hit this assertion then the __FILE__ macro is providing a
        // relative path instead of an absolute path. On Windows this will be
        // a path relative to the build directory rather than the currently
        // running application. To fix this you must compile with the /FC flag.
        jassert (File::isAbsolutePath (sourceFile));

        app->unhandledException (e, sourceFile, lineNumber);
    }
}

//==============================================================================
#if ! (JUCE_IOS || JUCE_ANDROID)
 #define JUCE_HANDLE_MULTIPLE_INSTANCES 1
#endif

#if JUCE_HANDLE_MULTIPLE_INSTANCES
struct JUCEApplicationBase::MultipleInstanceHandler final : public ActionListener
{
    MultipleInstanceHandler (const String& appName)
        : appLock ("juceAppLock_" + appName)
    {
    }

    bool sendCommandLineToPreexistingInstance()
    {
        if (appLock.enter (0))
            return false;

        if (auto* app = JUCEApplicationBase::getInstance())
        {
            MessageManager::broadcastMessage (app->getApplicationName() + "/" + app->getCommandLineParameters());
            return true;
        }

        jassertfalse;
        return false;
    }

    void actionListenerCallback (const String& message) override
    {
        if (auto* app = JUCEApplicationBase::getInstance())
        {
            auto appName = app->getApplicationName();

            if (message.startsWith (appName + "/"))
                app->anotherInstanceStarted (message.substring (appName.length() + 1));
        }
    }

private:
    InterProcessLock appLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultipleInstanceHandler)
};

bool JUCEApplicationBase::sendCommandLineToPreexistingInstance()
{
    jassert (multipleInstanceHandler == nullptr); // this must only be called once!

    multipleInstanceHandler.reset (new MultipleInstanceHandler (getApplicationName()));
    return multipleInstanceHandler->sendCommandLineToPreexistingInstance();
}

#else
struct JUCEApplicationBase::MultipleInstanceHandler {};
#endif

JUCEApplicationBase::JUCEApplicationBase()
{
    jassert (isStandaloneApp() && appInstance == nullptr);
    appInstance = this;
}

JUCEApplicationBase::~JUCEApplicationBase()
{
    jassert (appInstance == this);
    appInstance = nullptr;
}

//==============================================================================
#if JUCE_ANDROID

StringArray JUCEApplicationBase::getCommandLineParameterArray() { return {}; }
String JUCEApplicationBase::getCommandLineParameters()          { return {}; }

#else

#if JUCE_WINDOWS && ! defined (_CONSOLE)

String JUCE_CALLTYPE JUCEApplicationBase::getCommandLineParameters()
{
    return CharacterFunctions::findEndOfToken (CharPointer_UTF16 (GetCommandLineW()),
                                               CharPointer_UTF16 (L" "),
                                               CharPointer_UTF16 (L"\"")).findEndOfWhitespace();
}

StringArray JUCE_CALLTYPE JUCEApplicationBase::getCommandLineParameterArray()
{
    StringArray s;
    int argc = 0;

    if (auto argv = CommandLineToArgvW (GetCommandLineW(), &argc))
    {
        s = StringArray (argv + 1, argc - 1);
        LocalFree (argv);
    }

    return s;
}

#else

#if JUCE_IOS && JUCE_MODULE_AVAILABLE_juce_gui_basics
 extern int juce_iOSMain (int argc, const char* argv[], void* classPtr);
#endif

#if JUCE_MAC
 extern void initialiseNSApplication();
#endif

#if (JUCE_LINUX || JUCE_BSD) && JUCE_MODULE_AVAILABLE_juce_gui_extra && (! defined (JUCE_WEB_BROWSER) || JUCE_WEB_BROWSER)
 extern "C" int juce_gtkWebkitMain (int argc, const char* const* argv);
#endif

#if JUCE_WINDOWS
 const char* const* juce_argv = nullptr;
 int juce_argc = 0;
#else
 extern const char* const* juce_argv;  // declared in juce_core
 extern int juce_argc;
#endif

String JUCEApplicationBase::getCommandLineParameters()
{
    String argString;

    for (const auto& arg : getCommandLineParameterArray())
    {
        const auto withQuotes = arg.containsChar (' ') && ! arg.isQuotedString()
                              ? arg.quoted ('"')
                              : arg;
        argString << withQuotes << ' ';
    }

    return argString.trim();
}

StringArray JUCEApplicationBase::getCommandLineParameterArray()
{
    StringArray result;

    for (int i = 1; i < juce_argc; ++i)
        result.add (CharPointer_UTF8 (juce_argv[i]));

    return result;
}

int JUCEApplicationBase::main (int argc, const char* argv[])
{
    JUCE_AUTORELEASEPOOL
    {
        juce_argc = argc;
        juce_argv = argv;

       #if JUCE_MAC
        initialiseNSApplication();
       #endif

       #if (JUCE_LINUX || JUCE_BSD) && JUCE_MODULE_AVAILABLE_juce_gui_extra && (! defined (JUCE_WEB_BROWSER) || JUCE_WEB_BROWSER)
        if (argc >= 2 && String (argv[1]) == "--juce-gtkwebkitfork-child")
            return juce_gtkWebkitMain (argc, argv);
       #endif

       #if JUCE_IOS && JUCE_MODULE_AVAILABLE_juce_gui_basics
        return juce_iOSMain (argc, argv, iOSCustomDelegate);
       #else

        return JUCEApplicationBase::main();
       #endif
    }
}

#endif

//==============================================================================
int JUCEApplicationBase::main()
{
    ScopedJuceInitialiser_GUI libraryInitialiser;
    jassert (createInstance != nullptr);

    const std::unique_ptr<JUCEApplicationBase> app (createInstance());
    jassert (app != nullptr);

    if (! app->initialiseApp())
        return app->shutdownApp();

    JUCE_TRY
    {
        // loop until a quit message is received..
        MessageManager::getInstance()->runDispatchLoop();
    }
    JUCE_CATCH_EXCEPTION

    return app->shutdownApp();
}

#endif

//==============================================================================
bool JUCEApplicationBase::initialiseApp()
{
   #if JUCE_HANDLE_MULTIPLE_INSTANCES
    if ((! moreThanOneInstanceAllowed()) && sendCommandLineToPreexistingInstance())
    {
        DBG ("Another instance is running - quitting...");
        return false;
    }
   #endif

   #if JUCE_WINDOWS && (! defined (_CONSOLE))
    if (isStandaloneApp() && AttachConsole (ATTACH_PARENT_PROCESS) != 0)
    {
        // if we've launched a GUI app from cmd.exe or PowerShell, we need this to enable printf etc.
        // However, only reassign stdout, stderr, stdin if they have not been already opened by
        // a redirect or similar.
        FILE* ignore;

        if (_fileno (stdout) < 0) freopen_s (&ignore, "CONOUT$", "w", stdout);
        if (_fileno (stderr) < 0) freopen_s (&ignore, "CONOUT$", "w", stderr);
        if (_fileno (stdin)  < 0) freopen_s (&ignore, "CONIN$",  "r", stdin);
    }
   #endif

    // let the app do its setting-up..
    initialise (getCommandLineParameters());

    stillInitialising = false;

    if (MessageManager::getInstance()->hasStopMessageBeenSent())
        return false;

   #if JUCE_HANDLE_MULTIPLE_INSTANCES
    if (auto* mih = multipleInstanceHandler.get())
        MessageManager::getInstance()->registerBroadcastListener (mih);
   #endif

    return true;
}

int JUCEApplicationBase::shutdownApp()
{
    jassert (JUCEApplicationBase::getInstance() == this);

   #if JUCE_HANDLE_MULTIPLE_INSTANCES
    if (auto* mih = multipleInstanceHandler.get())
        MessageManager::getInstance()->deregisterBroadcastListener (mih);
   #endif

    JUCE_TRY
    {
        // give the app a chance to clean up..
        shutdown();
    }
    JUCE_CATCH_EXCEPTION

    multipleInstanceHandler.reset();
    return getApplicationReturnValue();
}

} // namespace juce
