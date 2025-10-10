/*
 * @Author: Punal Manalan
 * @Description: Android Browser Custom Tab Plugin.
 * @Date: 09/10/2025
 */

#include "CPP_ABCT_Base.h"
#include "Async/Async.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJavaEnv.h"
#endif

// ============================================================================
// External Declaration for Global Registry (defined in CPP_ABCT_Base.cpp)
// ============================================================================

#if PLATFORM_ANDROID
namespace ChromeCustomTabsRegistry
{
    extern UCPP_ABCT_Base *GetActiveInstance();
}
#endif

// ============================================================================
// JNI Callbacks - Called from Java
// ============================================================================

#if PLATFORM_ANDROID

extern "C"
{
    /**
     * JNI callback for Deep Links from Chrome Custom Tab.
     * Called from ChromeCustomTabs.java when a deep link is received.
     *
     * Method signature: nativeOnDeepLinkReceived(String action, String paramsJson)
     */
    JNIEXPORT void JNICALL Java_com_epicgames_unreal_customtabs_ChromeCustomTabs_nativeOnDeepLinkReceived(
        JNIEnv *Env,
        jclass Clazz,
        jstring jAction,
        jstring jParamsJson)
    {
        // Convert Java strings to C++ FStrings
        const char *actionChars = Env->GetStringUTFChars(jAction, nullptr);
        const char *paramsJsonChars = Env->GetStringUTFChars(jParamsJson, nullptr);

        FString Action = FString(UTF8_TO_TCHAR(actionChars));
        FString ParamsJson = FString(UTF8_TO_TCHAR(paramsJsonChars));

        // Release Java strings
        Env->ReleaseStringUTFChars(jAction, actionChars);
        Env->ReleaseStringUTFChars(jParamsJson, paramsJsonChars);

        UE_LOG(LogTemp, Log, TEXT("JNI: Deep Link received - Action=%s, Params=%s"), *Action, *ParamsJson);

        // Forward to the active UCPP_ABCT_Base instance on the game thread
        AsyncTask(ENamedThreads::GameThread, [Action, ParamsJson]()
                  {
            UCPP_ABCT_Base* Instance = ChromeCustomTabsRegistry::GetActiveInstance();
            if (Instance)
            {
                UE_LOG(LogTemp, Log, TEXT("JNI: Forwarding Deep Link to instance 0x%p"), Instance);
                Instance->HandleDeepLink(Action, ParamsJson);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("JNI: No active UCPP_ABCT_Base instance to receive Deep Link!"));
            } });
    }

    /**
     * JNI callback for Navigation Events from Chrome Custom Tab.
     * Called from ChromeCustomTabs.java when navigation events occur.
     *
     * Method signature: nativeOnNavigationEvent(int event, String url)
     */
    JNIEXPORT void JNICALL Java_com_epicgames_unreal_customtabs_ChromeCustomTabs_nativeOnNavigationEvent(
        JNIEnv *Env,
        jclass Clazz,
        jint jEvent,
        jstring jUrl)
    {
        // Convert Java string to C++ FString
        const char *urlChars = Env->GetStringUTFChars(jUrl, nullptr);
        FString URL = FString(UTF8_TO_TCHAR(urlChars));
        Env->ReleaseStringUTFChars(jUrl, urlChars);

        // Convert navigation event int to string
        FString EventName;
        switch (jEvent)
        {
        case 1:
            EventName = TEXT("NavigationStarted");
            break;
        case 2:
            EventName = TEXT("NavigationFinished");
            break;
        case 3:
            EventName = TEXT("NavigationFailed");
            break;
        case 4:
            EventName = TEXT("NavigationAborted");
            break;
        case 5:
            EventName = TEXT("TabShown");
            break;
        case 6:
            EventName = TEXT("TabHidden");
            break;
        default:
            EventName = FString::Printf(TEXT("Unknown(%d)"), jEvent);
            break;
        }

        UE_LOG(LogTemp, Log, TEXT("JNI: Navigation Event - %s, URL=%s"), *EventName, *URL);

        // Forward to the active UCPP_ABCT_Base instance on the game thread
        AsyncTask(ENamedThreads::GameThread, [EventName, URL]()
                  {
            UCPP_ABCT_Base* Instance = ChromeCustomTabsRegistry::GetActiveInstance();
            if (Instance)
            {
                Instance->HandleNavigationEvent(EventName, URL);
            } });
    }

    /**
     * JNI callback for Tab Opened event.
     * Called from ChromeCustomTabs.java when the tab is opened.
     *
     * Method signature: nativeOnTabOpened()
     */
    JNIEXPORT void JNICALL Java_com_epicgames_unreal_customtabs_ChromeCustomTabs_nativeOnTabOpened(
        JNIEnv *Env,
        jclass Clazz)
    {
        UE_LOG(LogTemp, Log, TEXT("JNI: Tab Opened"));

        AsyncTask(ENamedThreads::GameThread, []()
                  {
            UCPP_ABCT_Base* Instance = ChromeCustomTabsRegistry::GetActiveInstance();
            if (Instance)
            {
                Instance->HandleNavigationEvent(TEXT("TabOpened"), TEXT(""));
            } });
    }

    /**
     * JNI callback for Tab Closed event.
     * Called from ChromeCustomTabs.java when the tab is closed.
     *
     * Method signature: nativeOnTabClosed()
     */
    JNIEXPORT void JNICALL Java_com_epicgames_unreal_customtabs_ChromeCustomTabs_nativeOnTabClosed(
        JNIEnv *Env,
        jclass Clazz)
    {
        UE_LOG(LogTemp, Log, TEXT("JNI: Tab Closed"));

        AsyncTask(ENamedThreads::GameThread, []()
                  {
            UCPP_ABCT_Base* Instance = ChromeCustomTabsRegistry::GetActiveInstance();
            if (Instance)
            {
                Instance->HandleNavigationEvent(TEXT("TabClosed"), TEXT(""));
            } });
    }

    /**
     * JNI callback for PostMessage Channel Ready.
     * Called from ChromeCustomTabs.java when the PostMessage channel is ready.
     *
     * Method signature: nativeOnMessageChannelReady()
     */
    JNIEXPORT void JNICALL Java_com_epicgames_unreal_customtabs_ChromeCustomTabs_nativeOnMessageChannelReady(
        JNIEnv *Env,
        jclass Clazz)
    {
        UE_LOG(LogTemp, Log, TEXT("JNI: PostMessage Channel Ready"));

        AsyncTask(ENamedThreads::GameThread, []()
                  {
            UCPP_ABCT_Base* Instance = ChromeCustomTabsRegistry::GetActiveInstance();
            if (Instance)
            {
                Instance->HandleNavigationEvent(TEXT("MessageChannelReady"), TEXT(""));
            } });
    }

    /**
     * JNI callback for PostMessage from web page.
     * Called from ChromeCustomTabs.java when a message is received from the web page.
     *
     * Method signature: nativeOnPostMessage(String message, String origin)
     */
    JNIEXPORT void JNICALL Java_com_epicgames_unreal_customtabs_ChromeCustomTabs_nativeOnPostMessage(
        JNIEnv *Env,
        jclass Clazz,
        jstring jMessage,
        jstring jOrigin)
    {
        const char *messageChars = Env->GetStringUTFChars(jMessage, nullptr);
        const char *originChars = Env->GetStringUTFChars(jOrigin, nullptr);

        FString Message = FString(UTF8_TO_TCHAR(messageChars));
        FString Origin = FString(UTF8_TO_TCHAR(originChars));

        Env->ReleaseStringUTFChars(jMessage, messageChars);
        Env->ReleaseStringUTFChars(jOrigin, originChars);

        UE_LOG(LogTemp, Log, TEXT("JNI: PostMessage - Message=%s, Origin=%s"), *Message, *Origin);

        AsyncTask(ENamedThreads::GameThread, [Message, Origin]()
                  {
            UCPP_ABCT_Base* Instance = ChromeCustomTabsRegistry::GetActiveInstance();
            if (Instance)
            {
                // For now, just log. You can add a dedicated handler if needed.
                UE_LOG(LogTemp, Log, TEXT("PostMessage forwarded to instance 0x%p"), Instance);
            } });
    }
}

#endif // PLATFORM_ANDROID
