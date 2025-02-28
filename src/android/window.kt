// vim: set sw=2:
package __BUNDLE_IDENTIFIER__
import java.lang.ref.WeakReference

open class Window (runtime: Runtime, activity: MainActivity) {
  open protected val TAG = "Window"

  val bridge = Bridge(runtime, BridgeConfiguration(
    getRootDirectory = { ->
      this.getRootDirectory()
    }
  ))

  val userMessageHandler = UserMessageHandler(this)
  val activity = WeakReference(activity)
  val runtime = WeakReference(runtime)
  val pointer = alloc(bridge.pointer)

  fun evaluateJavaScript (source: String) {
    this.activity.get()?.evaluateJavaScript(source)
  }

  fun getRootDirectory (): String {
    return this.activity.get()?.getRootDirectory() ?: ""
  }

  fun load () {
    val isDebugEnabled = this.runtime.get()?.isDebugEnabled() ?: false
    val filename = this.getPathToFileToLoad()
    val activity = this.activity.get() ?: return

    val rootDirectory = this.getRootDirectory()
    this.bridge.route("ipc://internal.setcwd?value=${rootDirectory}", null, fun (result: Result) {
      activity.applicationContext
        .getSharedPreferences("WebSettings", android.app.Activity.MODE_PRIVATE)
        .edit()
        .apply {
          putString("scheme", "socket")
          putString("hostname", "__BUNDLE_IDENTIFIER__")
          apply()
        }

      activity.runOnUiThread {
        // enable/disable debug module in webview
        android.webkit.WebView.setWebContentsDebuggingEnabled(isDebugEnabled)

        activity.webview?.apply {
          // features
          settings.javaScriptEnabled = true
          settings.domStorageEnabled = true

          // allow list
          settings.allowFileAccess = true
          settings.allowContentAccess = true

          // allow mixed content
          settings.mixedContentMode = android.webkit.WebSettings.MIXED_CONTENT_ALWAYS_ALLOW

          activity.client.putRootDirectory(rootDirectory)
          webViewClient = activity.client

          addJavascriptInterface(userMessageHandler, "external")
          loadUrl("https://__BUNDLE_IDENTIFIER__/$filename")
        }
      }
    })
  }

  open fun onSchemeRequest (
    request: android.webkit.WebResourceRequest,
    response: android.webkit.WebResourceResponse,
    stream: java.io.PipedOutputStream
  ): Boolean {
    return bridge.route(request.url.toString(), null, fun (result: Result) {
      var bytes = result.value.toByteArray()
      var contentType = "application/json"

      if (result.bytes != null) {
        bytes = result.bytes
        contentType = "application/octet-stream"
      }

      response.apply {
        setStatusCodeAndReasonPhrase(200, "OK")
        setResponseHeaders(responseHeaders + result.headers + mapOf(
          "content-type" to contentType
        ))
        setMimeType(contentType)
      }

      kotlin.concurrent.thread {
        try {
          stream.write(bytes, 0, bytes.size)
        } catch (err: Exception) {
          if (err.message != "Pipe closed") {
            console.error("onSchemeRequest(): ${err.toString()}")
          }
        }

        stream.close()
      }
    })
  }

  open fun onPageStarted (
    view: android.webkit.WebView,
    url: String,
    bitmap: android.graphics.Bitmap?
  ) {
    val source = this.getJavaScriptPreloadSource()
    view.pauseTimers()
    this.activity.get()?.evaluateJavaScript(source)
    view.resumeTimers()
  }

  @Throws(java.lang.Exception::class)
  external fun alloc (bridgePointer: Long): Long;

  @Throws(java.lang.Exception::class)
  external fun dealloc (): Boolean;

  @Throws(java.lang.Exception::class)
  external fun getPathToFileToLoad (): String;

  @Throws(java.lang.Exception::class)
  external fun getJavaScriptPreloadSource (): String;

  @Throws(java.lang.Exception::class)
  external fun getResolveToRenderProcessJavaScript (
    seq: String,
    state: String,
    value: String
  ): String;
}

/**
 * External JavaScript interface attached to the webview at
 * `window.external`
 */
open class UserMessageHandler (window: Window) {
  val TAG = "UserMessageHandler"

  val namespace = "external"
  val activity = window.bridge.runtime.get()?.activity
  val runtime = window.bridge.runtime
  val window = WeakReference(window)

  @android.webkit.JavascriptInterface
  open fun postMessage (value: String): Boolean {
    return this.postMessage(value, null)
  }

  /**
   * Low level external message handler
   */
  @android.webkit.JavascriptInterface
  open fun postMessage (value: String, bytes: ByteArray? = null): Boolean {
    val bridge = this.window.get()?.bridge ?: return false

    return bridge.route(value, bytes, fun (result: Result) {
      val window = this.window.get() ?: return
      val javascript = window.getResolveToRenderProcessJavaScript(result.seq, "1", result.value)
      this.window.get()?.evaluateJavaScript(javascript)
    })
  }
}
