/*
* @Author: Punal Manalan
* @Description: Android Browser Custom Tab Plugin.
* @Date: 09/10/2025
*/

package com.epicgames.unreal.customtabs;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;

import androidx.annotation.Nullable;

import java.net.URLDecoder;
import java.io.UnsupportedEncodingException;
import androidx.browser.customtabs.CustomTabsCallback;
import androidx.browser.customtabs.CustomTabsClient;
import androidx.browser.customtabs.CustomTabsIntent;
import androidx.browser.customtabs.CustomTabsService;
import androidx.browser.customtabs.CustomTabsServiceConnection;
import androidx.browser.customtabs.CustomTabsSession;

import java.lang.ref.WeakReference;

public final class ChromeCustomTabs {
    private static final String TAG = "UEChromeTabs";

    private static WeakReference<Activity> activityRef = new WeakReference<>(null);
    private static CustomTabsClient customTabsClient;
    private static CustomTabsSession customTabsSession;
    private static CustomTabsServiceConnection serviceConnection;
    private static boolean messageChannelReady;
    private static Uri pendingOrigin;
    private static String lastNavigatedUrl = "";

    private ChromeCustomTabs() {
    }

    private static final CustomTabsCallback CALLBACK = new CustomTabsCallback() {
        @Override
        public void onNavigationEvent(int navigationEvent, Bundle extras) {
            String url = lastNavigatedUrl;
            // Note: EXTRA_URL constant was removed in newer AndroidX versions
            // The URL tracking is handled through lastNavigatedUrl instead
            nativeOnNavigationEvent(navigationEvent, url != null ? url : "");

            // Trigger PostMessage channel request when navigation finishes (if not already
            // ready)
            if (navigationEvent == CustomTabsCallback.NAVIGATION_FINISHED && !messageChannelReady
                    && pendingRetryOrigin != null) {
                Log.i(TAG, "Navigation finished - attempting PostMessage channel request");
                Activity activity = activityRef != null ? activityRef.get() : null;
                if (activity != null) {
                    retryHandler.postDelayed(() -> {
                        if (!messageChannelReady && pendingRetryOrigin != null) {
                            attemptPostMessageChannelRequest(activity, pendingRetryOrigin);
                        }
                    }, 500); // Wait 500ms after navigation finishes
                }
            }

            if (navigationEvent == TAB_SHOWN) {
                nativeOnTabOpened();
            }
            if (navigationEvent == TAB_HIDDEN) {
                // TAB_HIDDEN means the tab is still open but not visible (user switched apps)
                // This is NOT the same as closing the tab, so we don't call nativeOnTabClosed()
                Log.d(TAG, "Custom Tab is now hidden (but still open)");
                // Could add a separate callback here if needed: nativeOnTabHidden();
            }
            // Note: There's no reliable way to detect actual tab closure in Custom Tabs API
            // The tab close detection would need to be handled differently
        }

        @Override
        public void onMessageChannelReady(Bundle extras) {
            messageChannelReady = true;
            pendingRetryOrigin = null; // Stop any pending retries
            Log.i(TAG, "PostMessage channel is now ready!");
            nativeOnMessageChannelReady();
        }

        @Override
        public void onPostMessage(String message, Bundle extras) {
            String origin = pendingOrigin != null ? pendingOrigin.toString() : "";
            nativeOnPostMessage(message != null ? message : "", origin);
        }
    };

    public static synchronized void setActivity(@Nullable Activity activity) {
        activityRef = new WeakReference<>(activity);
        if (activity != null) {
            bindCustomTabs(activity.getApplicationContext());
        }
    }

    public static synchronized void onActivityDestroyed() {
        unbindCustomTabs();
        activityRef = new WeakReference<>(null);
    }

    public static synchronized boolean isAvailable(@Nullable Activity activity) {
        if (activity == null) {
            return false;
        }
        return getPackageNameToUse(activity) != null;
    }

    /**
     * Simplified method for C++ JNI - calls the full open() method with default
     * parameters
     */
    public static boolean openTab(String url, String toolbarColorHex, String userAgent, String customHeader) {
        Activity activity = activityRef != null ? activityRef.get() : null;
        if (activity == null) {
            Log.e(TAG, "openTab: Activity is NULL");
            return false;
        }

        // Parse hex color to int (e.g., "#4285F4" -> 0xFF4285F4)
        int toolbarColor = 0xFF4285F4; // Default Google Blue
        if (toolbarColorHex != null && toolbarColorHex.startsWith("#")) {
            try {
                toolbarColor = android.graphics.Color.parseColor(toolbarColorHex);
            } catch (IllegalArgumentException e) {
                Log.w(TAG, "Invalid toolbar color hex: " + toolbarColorHex, e);
            }
        }

        // Append query parameters to URL if userAgent or customHeader are set
        String finalUrl = url;
        if ((userAgent != null && !userAgent.isEmpty()) || (customHeader != null && !customHeader.isEmpty())) {
            Uri uri = Uri.parse(url);
            Uri.Builder builder = uri.buildUpon();

            // Add ue_client=true to indicate this is from Unreal Engine
            builder.appendQueryParameter("ue_client", "true");

            // Add custom user agent as query parameter if provided
            if (userAgent != null && !userAgent.isEmpty()) {
                builder.appendQueryParameter("ue_user_agent", userAgent);
                Log.i(TAG, "Adding custom user agent query parameter: " + userAgent);
            }

            // Add custom header as query parameter if provided
            if (customHeader != null && !customHeader.isEmpty()) {
                builder.appendQueryParameter("ue_custom_header", customHeader);
                Log.i(TAG, "Adding custom header query parameter: " + customHeader);
            }

            finalUrl = builder.build().toString();
            Log.i(TAG, "Modified URL with query parameters: " + finalUrl);
        }

        // Call full open() method with default parameters
        return open(activity, finalUrl, toolbarColor, false, 0, true, true, true, null);
    }

    public static boolean open(final Activity activity,
            final String url,
            final int toolbarColor,
            final boolean hasNavigationBarColor,
            final int navigationBarColor,
            final boolean showTitle,
            final boolean enableUrlBarHiding,
            final boolean enableDefaultShareMenu,
            @Nullable final String postMessageOrigin) {
        if (activity == null || url == null || url.isEmpty()) {
            return false;
        }

        if (!isAvailable(activity)) {
            Log.w(TAG, "Chrome Custom Tabs service unavailable on this device. Falling back to ACTION_VIEW intent.");
            try {
                Intent fallbackIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
                fallbackIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                activity.startActivity(fallbackIntent);
                return true;
            } catch (ActivityNotFoundException e) {
                Log.w(TAG, "Fallback ACTION_VIEW intent failed", e);
                return false;
            }
        }

        ensureSession(activity);
        lastNavigatedUrl = url;
        activity.runOnUiThread(() -> openInternal(activity, url, toolbarColor, hasNavigationBarColor,
                navigationBarColor, showTitle, enableUrlBarHiding, enableDefaultShareMenu, postMessageOrigin));
        return true;
    }

    private static int postMessageRetryCount = 0;
    private static final int MAX_POSTMESSAGE_RETRIES = 5;
    private static final int RETRY_DELAY_MS = 1000; // 1 second between retries
    private static android.os.Handler retryHandler = new android.os.Handler(android.os.Looper.getMainLooper());
    private static String pendingRetryOrigin = null;

    public static boolean requestPostMessageChannel(Activity activity, String origin) {
        if (activity == null || origin == null || origin.isEmpty()) {
            return false;
        }

        ensureSession(activity);
        if (customTabsSession == null) {
            Log.w(TAG, "Cannot request postMessage channel without a session.");
            return false;
        }

        pendingOrigin = Uri.parse(origin);
        pendingRetryOrigin = origin;
        messageChannelReady = false;
        postMessageRetryCount = 0;

        activity.runOnUiThread(() -> {
            attemptPostMessageChannelRequest(activity, origin);
        });
        return true;
    }

    private static void attemptPostMessageChannelRequest(Activity activity, String origin) {
        if (customTabsSession == null) {
            Log.w(TAG, "Session unavailable for PostMessage channel request");
            return;
        }

        boolean result = customTabsSession.requestPostMessageChannel(Uri.parse(origin));
        postMessageRetryCount++;

        if (!result) {
            Log.w(TAG, String.format(
                    "requestPostMessageChannel returned false (attempt %d/%d). The session may not be ready yet.",
                    postMessageRetryCount, MAX_POSTMESSAGE_RETRIES));

            if (postMessageRetryCount < MAX_POSTMESSAGE_RETRIES) {
                Log.i(TAG, String.format("Will retry PostMessage channel request in %d ms...", RETRY_DELAY_MS));
                retryHandler.postDelayed(() -> {
                    if (!messageChannelReady && pendingRetryOrigin != null) {
                        attemptPostMessageChannelRequest(activity, pendingRetryOrigin);
                    }
                }, RETRY_DELAY_MS);
            } else {
                Log.e(TAG, "Failed to establish PostMessage channel after " + MAX_POSTMESSAGE_RETRIES + " attempts");
                pendingRetryOrigin = null;
            }
        } else {
            Log.i(TAG, String.format("requestPostMessageChannel succeeded on attempt %d", postMessageRetryCount));
            pendingRetryOrigin = null;
        }
    }

    public static boolean executeJava(String message) {
        if (customTabsSession == null) {
            Log.w(TAG, "No active custom tabs session. ExecuteJava will be ignored.");
            return false;
        }
        if (!messageChannelReady) {
            Log.w(TAG, "Message channel not ready. Call RequestPostMessageChannel first and wait for readiness.");
            return false;
        }
        int result = customTabsSession.postMessage(message, null);
        return result == CustomTabsService.RESULT_SUCCESS;
    }

    /**
     * Simplified method for C++ JNI - closes the custom tab
     */
    public static void closeTab() {
        Activity activity = activityRef != null ? activityRef.get() : null;
        if (activity != null) {
            close(activity);
        }
    }

    public static void close(Activity activity) {
        if (activity == null) {
            return;
        }

        activity.runOnUiThread(() -> {
            Intent intent = new Intent(activity, activity.getClass());
            intent.addFlags(
                    Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
            activity.startActivity(intent);
        });
    }

    private static void openInternal(Activity activity,
            String url,
            int toolbarColor,
            boolean hasNavigationBarColor,
            int navigationBarColor,
            boolean showTitle,
            boolean enableUrlBarHiding,
            boolean enableDefaultShareMenu,
            @Nullable String postMessageOrigin) {
        ensureSession(activity);

        CustomTabsIntent.Builder builder = customTabsSession != null ? new CustomTabsIntent.Builder(customTabsSession)
                : new CustomTabsIntent.Builder();
        builder.setToolbarColor(toolbarColor);
        builder.setShowTitle(showTitle);
        // Note: setUrlBarHidingEnabled was deprecated and removed in newer versions
        // Note: setNavigationBarColor was deprecated and removed in newer versions
        // Note: Share menu methods have changed across versions - omitting for
        // compatibility

        CustomTabsIntent intent = builder.build();
        intent.intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        intent.launchUrl(activity, Uri.parse(url));

        if (customTabsSession != null && postMessageOrigin != null && !postMessageOrigin.isEmpty()) {
            requestPostMessageChannel(activity, postMessageOrigin);
        }
    }

    private static synchronized void bindCustomTabs(Context context) {
        if (serviceConnection != null) {
            return;
        }

        String packageName = getPackageNameToUse(context);
        if (packageName == null) {
            Log.w(TAG, "Unable to bind Chrome Custom Tabs service (no supporting browser).");
            return;
        }

        serviceConnection = new CustomTabsServiceConnection() {
            @Override
            public void onCustomTabsServiceConnected(ComponentName name, CustomTabsClient client) {
                customTabsClient = client;
                customTabsClient.warmup(0L);
                customTabsSession = customTabsClient.newSession(CALLBACK);
                messageChannelReady = false;
            }

            @Override
            public void onServiceDisconnected(ComponentName componentName) {
                customTabsClient = null;
                customTabsSession = null;
                messageChannelReady = false;
            }
        };

        CustomTabsClient.bindCustomTabsService(context, packageName, serviceConnection);
    }

    private static synchronized void unbindCustomTabs() {
        Activity activity = activityRef.get();
        if (activity != null && serviceConnection != null) {
            activity.getApplicationContext().unbindService(serviceConnection);
        }
        serviceConnection = null;
        customTabsClient = null;
        customTabsSession = null;
        messageChannelReady = false;
        pendingOrigin = null;
    }

    private static synchronized void ensureSession(Activity activity) {
        if (customTabsSession != null) {
            return;
        }
        bindCustomTabs(activity.getApplicationContext());
        if (customTabsClient != null && customTabsSession == null) {
            customTabsSession = customTabsClient.newSession(CALLBACK);
        }
    }

    @Nullable
    private static String getPackageNameToUse(Context context) {
        return CustomTabsClient.getPackageName(context, null);
    }

    /**
     * Handle Deep Link intents from Custom Tab back to the app
     * Call this from GameActivity's onNewIntent to process incoming Deep Links
     */
    public static boolean handleDeepLink(Intent intent) {
        Log.d(TAG, "handleDeepLink called");

        if (intent == null) {
            Log.d(TAG, "handleDeepLink: Intent is NULL");
            return false;
        }

        Uri data = intent.getData();
        if (data == null) {
            Log.d(TAG, "handleDeepLink: Intent.getData() is NULL. Intent action=" + intent.getAction()
                    + ", flags=0x" + Integer.toHexString(intent.getFlags()));
            return false;
        }

        String scheme = data.getScheme();
        if (scheme == null) {
            Log.d(TAG, "handleDeepLink: Invalid scheme: NULL");
            return false;
        }

        // Extract Deep Link data
        String action = data.getHost(); // e.g., "message", "jump", "teleport"
        String query = data.getQuery(); // e.g., "text=hello&priority=high"

        String fullUrl = data.toString();
        Log.i(TAG, "Deep Link received: " + fullUrl);
        Log.i(TAG, "Action: " + action + ", Query: " + query);

        // Convert query string to JSON format for C++
        String paramsJson = queryStringToJson(query);

        // Notify native code
        nativeOnDeepLinkReceived(action != null ? action : "", paramsJson);
        return true;
    }

    /**
     * Convert URL query string to JSON format
     * Example: "text=hello&priority=high" -> "{"text":"hello","priority":"high"}"
     */
    private static String queryStringToJson(String query) {
        if (query == null || query.isEmpty()) {
            return "{}";
        }

        StringBuilder json = new StringBuilder("{");
        String[] pairs = query.split("&");
        for (int i = 0; i < pairs.length; i++) {
            String[] keyValue = pairs[i].split("=", 2);
            if (keyValue.length == 2) {
                if (i > 0)
                    json.append(",");

                // Decode URL-encoded value
                String decodedValue = keyValue[1];
                try {
                    decodedValue = URLDecoder.decode(keyValue[1], "UTF-8");
                } catch (UnsupportedEncodingException e) {
                    // UTF-8 is always supported, but just in case keep original
                }

                json.append("\"").append(keyValue[0]).append("\":\"")
                        .append(decodedValue).append("\"");
            }
        }
        json.append("}");
        return json.toString();
    }

    private static native void nativeOnTabOpened();

    private static native void nativeOnTabClosed();

    private static native void nativeOnNavigationEvent(int event, String url);

    private static native void nativeOnMessageChannelReady();

    private static native void nativeOnPostMessage(String message, String origin);

    private static native void nativeOnDeepLinkReceived(String action, String paramsJson);
}
