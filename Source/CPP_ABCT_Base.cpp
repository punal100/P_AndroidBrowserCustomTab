// Copyright Epic Games, Inc. All Rights Reserved.

#include "CPP_ABCT_Base.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if PLATFORM_ANDROID || defined(__ANDROID__)
#include "Android/AndroidApplication.h"
#include "Android/AndroidJavaEnv.h"
#endif

// ============================================================================
// Global Registry for Active Chrome Custom Tab Instance (Android-only)
// ============================================================================

#if PLATFORM_ANDROID
namespace ChromeCustomTabsRegistry
{
    // The UCPP_ABCT_Base instance that currently has an open Chrome Custom Tab
    static TWeakObjectPtr<UCPP_ABCT_Base> ActiveInstance = nullptr;

    void RegisterActiveInstance(UCPP_ABCT_Base *Instance)
    {
        ActiveInstance = Instance;
        UE_LOG(LogTemp, Log, TEXT("ChromeCustomTabsRegistry: Registered instance 0x%p as active"), Instance);
    }

    void UnregisterActiveInstance()
    {
        UE_LOG(LogTemp, Log, TEXT("ChromeCustomTabsRegistry: Unregistered active instance"));
        ActiveInstance = nullptr;
    }

    UCPP_ABCT_Base *GetActiveInstance()
    {
        return ActiveInstance.IsValid() ? ActiveInstance.Get() : nullptr;
    }
}
#endif

// ============================================================================
// Constructor
// ============================================================================

UCPP_ABCT_Base::UCPP_ABCT_Base()
{
    // Initialize state variables
    bIsCustomTabOpen = false;
    CurrentURL = TEXT("");
    LastNavigationEvent = TEXT("");
    LastDeepLinkAction = TEXT("");
    LastDeepLinkParams = TEXT("");

    // Initialize configuration variables with defaults
    DefaultToolbarColor = TEXT("#4285F4"); // Google Blue
    bShowTitle = true;
    bEnableUrlBarHiding = true;
    CustomUserAgent = TEXT(""); // Empty = use default browser user agent
    CustomHeader = TEXT("");    // Empty = no custom header

    // Initialize debug settings
    bEnableDebugLogging = true; // Enable by default for development

    DebugLog(TEXT("UCPP_ABCT_Base initialized"));
}

// ============================================================================
// Chrome Custom Tab - Opening URLs
// ============================================================================

bool UCPP_ABCT_Base::OpenChromeCustomTab(const FString &URL, const FString &ToolbarColor)
{
    DebugLog(FString::Printf(TEXT("OpenChromeCustomTab called with URL: %s, Color: %s"), *URL, *ToolbarColor));

    if (URL.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("UCPP_ABCT_Base::OpenChromeCustomTab - URL is empty!"));
        return false;
    }

#if PLATFORM_ANDROID
    // Call Java method to open Chrome Custom Tab
    if (JNIEnv *Env = FAndroidApplication::GetJavaEnv())
    {
        // Find ChromeCustomTabs class
        jclass ChromeCustomTabsClass = FAndroidApplication::FindJavaClass("com/epicgames/unreal/customtabs/ChromeCustomTabs");
        if (ChromeCustomTabsClass != nullptr)
        {
            // Find openTab static method with new signature (includes userAgent and customHeader)
            jmethodID OpenTabMethod = Env->GetStaticMethodID(ChromeCustomTabsClass, "openTab", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z");
            if (OpenTabMethod != nullptr)
            {
                // Convert FString to jstring
                jstring jURL = Env->NewStringUTF(TCHAR_TO_UTF8(*URL));
                jstring jColor = Env->NewStringUTF(TCHAR_TO_UTF8(*ToolbarColor));
                jstring jUserAgent = Env->NewStringUTF(TCHAR_TO_UTF8(*CustomUserAgent));
                jstring jCustomHeader = Env->NewStringUTF(TCHAR_TO_UTF8(*CustomHeader));

                // Call Java method
                jboolean result = Env->CallStaticBooleanMethod(ChromeCustomTabsClass, OpenTabMethod, jURL, jColor, jUserAgent, jCustomHeader);

                // Clean up local references
                Env->DeleteLocalRef(jURL);
                Env->DeleteLocalRef(jColor);
                Env->DeleteLocalRef(jUserAgent);
                Env->DeleteLocalRef(jCustomHeader);
                Env->DeleteLocalRef(ChromeCustomTabsClass);

                if (result)
                {
                    OnCustomTabOpened(URL);
                    DebugLog(TEXT("Chrome Custom Tab opened successfully"));
                    return true;
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("UCPP_ABCT_Base::OpenChromeCustomTab - Java method returned false"));
                    return false;
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("UCPP_ABCT_Base::OpenChromeCustomTab - openTab method not found"));
                Env->DeleteLocalRef(ChromeCustomTabsClass);
                return false;
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UCPP_ABCT_Base::OpenChromeCustomTab - ChromeCustomTabs class not found"));
            return false;
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UCPP_ABCT_Base::OpenChromeCustomTab - Failed to get JNI environment"));
        return false;
    }
#else
    UE_LOG(LogTemp, Warning, TEXT("UCPP_ABCT_Base::OpenChromeCustomTab - Not running on Android platform"));
    return false;
#endif
}

void UCPP_ABCT_Base::CloseChromeCustomTab()
{
    DebugLog(TEXT("CloseChromeCustomTab called"));

#if PLATFORM_ANDROID
    // Call Java method to close Chrome Custom Tab
    if (JNIEnv *Env = FAndroidApplication::GetJavaEnv())
    {
        jclass ChromeCustomTabsClass = FAndroidApplication::FindJavaClass("com/epicgames/unreal/customtabs/ChromeCustomTabs");
        if (ChromeCustomTabsClass != nullptr)
        {
            jmethodID CloseTabMethod = Env->GetStaticMethodID(ChromeCustomTabsClass, "closeTab", "()V");
            if (CloseTabMethod != nullptr)
            {
                Env->CallStaticVoidMethod(ChromeCustomTabsClass, CloseTabMethod);
                OnCustomTabClosed();
                DebugLog(TEXT("Chrome Custom Tab closed"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("UCPP_ABCT_Base::CloseChromeCustomTab - closeTab method not found"));
            }
            Env->DeleteLocalRef(ChromeCustomTabsClass);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("UCPP_ABCT_Base::CloseChromeCustomTab - ChromeCustomTabs class not found"));
        }
    }
#else
    UE_LOG(LogTemp, Warning, TEXT("UCPP_ABCT_Base::CloseChromeCustomTab - Not running on Android platform"));
#endif
}

// ============================================================================
// Chrome Custom Tab - Navigation Events
// ============================================================================

void UCPP_ABCT_Base::HandleNavigationEvent(const FString &Event, const FString &URL)
{
    DebugLog(FString::Printf(TEXT("HandleNavigationEvent: Event=%s, URL=%s"), *Event, *URL));

    // Update internal state
    LastNavigationEvent = Event;
    if (!URL.IsEmpty())
    {
        CurrentURL = URL;
    }

    // Handle specific events
    if (Event == TEXT("TabClosed"))
    {
        OnCustomTabClosed();
    }
    else if (Event == TEXT("NavigationStarted") && bIsCustomTabOpen == false)
    {
        OnCustomTabOpened(URL);
    }

    // Broadcast to Blueprint
    OnNavigationEvent(Event, URL);
}

// ============================================================================
// Deep Link - Receiving from Web Pages
// ============================================================================

void UCPP_ABCT_Base::HandleDeepLink(const FString &Action, const FString &ParamsJson)
{
    DebugLog(FString::Printf(TEXT("HandleDeepLink: Action=%s, Params=%s"), *Action, *ParamsJson));

    // Update internal state
    LastDeepLinkAction = Action;
    LastDeepLinkParams = ParamsJson;

    // Broadcast to Blueprint
    OnDeepLinkReceived(Action, ParamsJson);
}

// ============================================================================
// Deep Link - Parameter Parsing Helpers
// ============================================================================

bool UCPP_ABCT_Base::GetDeepLinkParameter(const FString &ParamsJson, const FString &Key, FString &OutValue)
{
    if (ParamsJson.IsEmpty() || Key.IsEmpty())
    {
        return false;
    }

    // Parse JSON string
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ParamsJson);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        // Try to get the value
        if (JsonObject->HasField(Key))
        {
            OutValue = JsonObject->GetStringField(Key);
            DebugLog(FString::Printf(TEXT("GetDeepLinkParameter: Key=%s, Value=%s"), *Key, *OutValue));
            return true;
        }
        else
        {
            DebugLog(FString::Printf(TEXT("GetDeepLinkParameter: Key=%s not found in JSON"), *Key));
            return false;
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("UCPP_ABCT_Base::GetDeepLinkParameter - Failed to parse JSON: %s"), *ParamsJson);
        return false;
    }
}

bool UCPP_ABCT_Base::GetDeepLinkParameterAsFloat(const FString &ParamsJson, const FString &Key, float &OutValue)
{
    FString StringValue;
    if (GetDeepLinkParameter(ParamsJson, Key, StringValue))
    {
        OutValue = FCString::Atof(*StringValue);
        return true;
    }
    return false;
}

bool UCPP_ABCT_Base::GetDeepLinkParameterAsInt(const FString &ParamsJson, const FString &Key, int32 &OutValue)
{
    FString StringValue;
    if (GetDeepLinkParameter(ParamsJson, Key, StringValue))
    {
        OutValue = FCString::Atoi(*StringValue);
        return true;
    }
    return false;
}

bool UCPP_ABCT_Base::GetDeepLinkParameterAsVector(const FString &ParamsJson, FVector &OutVector)
{
    float X, Y, Z;
    bool bFoundX = GetDeepLinkParameterAsFloat(ParamsJson, TEXT("x"), X);
    bool bFoundY = GetDeepLinkParameterAsFloat(ParamsJson, TEXT("y"), Y);
    bool bFoundZ = GetDeepLinkParameterAsFloat(ParamsJson, TEXT("z"), Z);

    if (bFoundX && bFoundY && bFoundZ)
    {
        OutVector = FVector(X, Y, Z);
        DebugLog(FString::Printf(TEXT("GetDeepLinkParameterAsVector: X=%f, Y=%f, Z=%f"), X, Y, Z));
        return true;
    }
    else
    {
        DebugLog(FString::Printf(TEXT("GetDeepLinkParameterAsVector: Failed to extract all components (X=%d, Y=%d, Z=%d)"), bFoundX, bFoundY, bFoundZ));
        return false;
    }
}

// ============================================================================
// Internal Helper Functions
// ============================================================================

void UCPP_ABCT_Base::DebugLog(const FString &Message)
{
    if (bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("UCPP_ABCT_Base: %s"), *Message);
    }
}

void UCPP_ABCT_Base::OnCustomTabOpened(const FString &URL)
{
    bIsCustomTabOpen = true;
    CurrentURL = URL;
    DebugLog(FString::Printf(TEXT("Custom Tab opened: %s"), *URL));

    // Register this instance with the global registry so it receives deep links
#if PLATFORM_ANDROID
    ChromeCustomTabsRegistry::RegisterActiveInstance(this);
#endif
}

void UCPP_ABCT_Base::OnCustomTabClosed()
{
    bIsCustomTabOpen = false;
    CurrentURL = TEXT("");
    DebugLog(TEXT("Custom Tab closed"));

    // Unregister this instance from the global registry
#if PLATFORM_ANDROID
    ChromeCustomTabsRegistry::UnregisterActiveInstance();
#endif
}
