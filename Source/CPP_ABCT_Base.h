/*
 * @Author: Punal Manalan
 * @Description: Android Browser Custom Tab Plugin.
 * @Date: 09/10/2025
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CPP_ABCT_Base.generated.h"

/**
 * UCPP_ABCT_Base
 *
 * Base UObject class for Chrome Custom Tab (ABCT) implementation.
 * Handles communication between Unreal Engine and Android Chrome Custom Tabs.
 *
 * Features:
 * - Opening URLs in Chrome Custom Tab overlay
 * - Receiving navigation events from Chrome Custom Tab
 * - Processing Deep Links from web pages back to the app
 * - Managing Custom Tab lifecycle (open, close, hidden, shown)
 *
 * Deep Link Format: uewebtest://action?param1=value1&param2=value2
 * Example: uewebtest://teleport?x=1000&y=0&z=500
 */
UCLASS(Blueprintable, BlueprintType)
class P_ANDROIDBROWSERCUSTOMTAB_API UCPP_ABCT_Base : public UObject
{
    GENERATED_BODY()

public:
    // Constructor
    UCPP_ABCT_Base();

    // ============================================================================
    // Chrome Custom Tab - Opening URLs
    // ============================================================================

    /**
     * Opens a URL in Chrome Custom Tab overlay.
     *
     * @param URL - The web address to open (e.g., "http://192.168.1.8:8080")
     * @param ToolbarColor - Custom toolbar color in hex format (e.g., "#4285F4" for blue)
     * @return true if Custom Tab opened successfully, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "Punal|Android|Browser|Chrome Custom Tab")
    bool OpenChromeCustomTab(const FString &URL, const FString &ToolbarColor = "#4285F4");

    /**
     * Closes the currently open Chrome Custom Tab.
     */
    UFUNCTION(BlueprintCallable, Category = "Punal|Android|Browser|Chrome Custom Tab")
    void CloseChromeCustomTab();

    // ============================================================================
    // Chrome Custom Tab - Navigation Events
    // ============================================================================

    /**
     * Called when navigation events occur in the Chrome Custom Tab.
     * Events include: NavigationStarted, NavigationFinished, NavigationFailed, NavigationAborted,
     *                 TabClosed, TabHidden, TabShown
     *
     * @param Event - The type of navigation event
     * @param URL - The URL associated with the event
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Punal|Android|Browser|Chrome Custom Tab")
    void OnNavigationEvent(const FString &Event, const FString &URL);

    /**
     * Native handler for navigation events from Java.
     * Converts from C++ to Blueprint event.
     *
     * @param Event - The type of navigation event
     * @param URL - The URL associated with the event
     */
    void HandleNavigationEvent(const FString &Event, const FString &URL);

    // ============================================================================
    // Deep Link - Receiving from Web Pages
    // ============================================================================

    /**
     * Called when a Deep Link is received from a web page.
     * Web pages can trigger this by navigating to: uewebtest://action?params
     *
     * Example Deep Links:
     * - uewebtest://message?text=Hello&priority=high
     * - uewebtest://teleport?x=1000&y=0&z=500
     * - uewebtest://jump?height=500
     *
     * @param Action - The action to perform (e.g., "message", "teleport", "jump")
     * @param ParamsJson - JSON string containing parameters (e.g., {"x":"1000","y":"0","z":"500"})
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Punal|Android|Browser|Chrome Custom Tab|Deep Link")
    void OnDeepLinkReceived(const FString &Action, const FString &ParamsJson);

    /**
     * Native handler for Deep Links from Java.
     * Converts from C++ to Blueprint event.
     *
     * @param Action - The action to perform
     * @param ParamsJson - JSON string containing parameters
     */
    void HandleDeepLink(const FString &Action, const FString &ParamsJson);

    // ============================================================================
    // Deep Link - Parameter Parsing Helpers
    // ============================================================================

    /**
     * Parses a JSON parameter string and extracts a specific value.
     *
     * @param ParamsJson - JSON string (e.g., {"x":"1000","y":"0","z":"500"})
     * @param Key - The parameter key to extract (e.g., "x")
     * @param OutValue - The extracted value as a string
     * @return true if the key was found, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "Punal|Android|Browser|Chrome Custom Tab|Deep Link")
    bool GetDeepLinkParameter(const FString &ParamsJson, const FString &Key, FString &OutValue);

    /**
     * Parses a JSON parameter string and extracts a float value.
     *
     * @param ParamsJson - JSON string
     * @param Key - The parameter key to extract
     * @param OutValue - The extracted value as a float
     * @return true if the key was found and parsed successfully, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "Punal|Android|Browser|Chrome Custom Tab|Deep Link")
    bool GetDeepLinkParameterAsFloat(const FString &ParamsJson, const FString &Key, float &OutValue);

    /**
     * Parses a JSON parameter string and extracts an integer value.
     *
     * @param ParamsJson - JSON string
     * @param Key - The parameter key to extract
     * @param OutValue - The extracted value as an integer
     * @return true if the key was found and parsed successfully, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "Punal|Android|Browser|Chrome Custom Tab|Deep Link")
    bool GetDeepLinkParameterAsInt(const FString &ParamsJson, const FString &Key, int32 &OutValue);

    /**
     * Parses a JSON parameter string and extracts a vector (x, y, z).
     *
     * @param ParamsJson - JSON string (must contain "x", "y", "z" keys)
     * @param OutVector - The extracted vector
     * @return true if all three components were found and parsed successfully, false otherwise
     */
    UFUNCTION(BlueprintCallable, Category = "Punal|Android|Browser|Chrome Custom Tab|Deep Link")
    bool GetDeepLinkParameterAsVector(const FString &ParamsJson, FVector &OutVector);

    // ============================================================================
    // State Management
    // ============================================================================

    /**
     * Returns whether a Chrome Custom Tab is currently open.
     *
     * @return true if Custom Tab is open, false otherwise
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Punal|Android|Browser|Chrome Custom Tab")
    bool IsChromeCustomTabOpen() const { return bIsCustomTabOpen; }

    /**
     * Returns the current URL displayed in the Chrome Custom Tab.
     *
     * @return The current URL, or empty string if no tab is open
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Punal|Android|Browser|Chrome Custom Tab")
    FString GetCurrentURL() const { return CurrentURL; }

protected:
    // ============================================================================
    // Internal State Variables
    // ============================================================================

    /** Whether a Chrome Custom Tab is currently open */
    UPROPERTY(BlueprintReadOnly, Category = "Punal|Android|Browser|Chrome Custom Tab")
    bool bIsCustomTabOpen;

    /** The current URL displayed in the Chrome Custom Tab */
    UPROPERTY(BlueprintReadOnly, Category = "Punal|Android|Browser|Chrome Custom Tab")
    FString CurrentURL;

    /** The last navigation event received */
    UPROPERTY(BlueprintReadOnly, Category = "Punal|Android|Browser|Chrome Custom Tab")
    FString LastNavigationEvent;

    /** The last Deep Link action received */
    UPROPERTY(BlueprintReadOnly, Category = "Punal|Android|Browser|Chrome Custom Tab")
    FString LastDeepLinkAction;

    /** The last Deep Link parameters received (as JSON string) */
    UPROPERTY(BlueprintReadOnly, Category = "Punal|Android|Browser|Chrome Custom Tab")
    FString LastDeepLinkParams;

    // ============================================================================
    // Configuration Variables
    // ============================================================================

    /** Default toolbar color for Chrome Custom Tab (hex format: #RRGGBB) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Punal|Android|Browser|Chrome Custom Tab|Config")
    FString DefaultToolbarColor;

    /** Whether to show the page title in the Custom Tab toolbar */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Punal|Android|Browser|Chrome Custom Tab|Config")
    bool bShowTitle;

    /** Whether to enable URL hiding in the Custom Tab */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Punal|Android|Browser|Chrome Custom Tab|Config")
    bool bEnableUrlBarHiding;

    /** Custom user agent string for Chrome Custom Tab (empty = use default browser user agent) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Punal|Android|Browser|Chrome Custom Tab|Config")
    FString CustomUserAgent;

    /** Custom HTTP header to append to requests (empty = no custom header) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Punal|Android|Browser|Chrome Custom Tab|Config")
    FString CustomHeader;

    // ============================================================================
    // Debug Variables
    // ============================================================================

    /** Enable debug logging for Chrome Custom Tab events */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chrome Custom Tab|Debug")
    bool bEnableDebugLogging;

private:
    // ============================================================================
    // Internal Helper Functions
    // ============================================================================

    /**
     * Logs a debug message if debug logging is enabled.
     *
     * @param Message - The message to log
     */
    void DebugLog(const FString &Message);

    /**
     * Updates internal state when Custom Tab opens.
     *
     * @param URL - The URL that was opened
     */
    void OnCustomTabOpened(const FString &URL);

    /**
     * Updates internal state when Custom Tab closes.
     */
    void OnCustomTabClosed();
};
