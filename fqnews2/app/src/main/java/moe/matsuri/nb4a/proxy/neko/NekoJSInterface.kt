package moe.matsuri.nb4a.proxy.neko

import android.annotation.SuppressLint
import android.webkit.*
import android.widget.Toast
import androidx.preference.Preference
import androidx.preference.PreferenceScreen
import com.nononsenseapps.feeder.BuildConfig
import jww.app.FeederApplication
import io.nekohasekai.sagernet.database.DataStore
import io.nekohasekai.sagernet.ktx.*
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.withContext
import moe.matsuri.nb4a.plugin.NekoPluginManager
import moe.matsuri.nb4a.utils.JavaUtil
import moe.matsuri.nb4a.utils.Util
import moe.matsuri.nb4a.utils.WebViewUtil
import org.json.JSONObject
import java.io.File
import java.io.FileInputStream
import java.util.*
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException
import kotlin.coroutines.suspendCoroutine

class NekoJSInterface(val plgId: String) {

    private val mutex = Mutex()
    private var webView: WebView? = null
    val jsObject = JsObject()
    var plgConfig: JSONObject? = null
    var plgConfigException: Exception? = null
    val protocols = mutableMapOf<String, NekoProtocol>()
    val loaded = AtomicBoolean()

    suspend fun lock() {
        mutex.lock(null)
    }

    fun unlock() {
        mutex.unlock(null)
    }

    // load webview and js
    // Return immediately when already loaded
    // Return plgConfig or throw exception
    suspend fun init() = withContext(Dispatchers.Main) {
        initInternal()
    }

    @SuppressLint("SetJavaScriptEnabled")
    private suspend fun initInternal() = suspendCoroutine<JSONObject> {
        if (loaded.get()) {
            plgConfig?.apply {
                it.resume(this)
                return@suspendCoroutine
            }
            plgConfigException?.apply {
                it.resumeWithException(this)
                return@suspendCoroutine
            }
            it.resumeWithException(Exception("wtf"))
            return@suspendCoroutine
        }

        WebView.setWebContentsDebuggingEnabled(BuildConfig.DEBUG)
        NekoPluginManager.extractPlugin(plgId, false)

        webView = WebView(FeederApplication.instance.applicationContext)
        webView!!.settings.javaScriptEnabled = true
        webView!!.addJavascriptInterface(jsObject, "neko")
        webView!!.webViewClient = object : WebViewClient() {
            // provide files
            override fun shouldInterceptRequest(
                view: WebView?, request: WebResourceRequest?
            ): WebResourceResponse {
                return WebViewUtil.interceptRequest(
                    { res ->
                        val f = File(NekoPluginManager.htmlPath(plgId), res)
                        if (f.exists()) {
                            FileInputStream(f)
                        } else {
                            null
                        }
                    },
                    view,
                    request
                )
            }

            override fun onReceivedError(
                view: WebView?, request: WebResourceRequest?, error: WebResourceError?
            ) {
                WebViewUtil.onReceivedError(view, request, error)
            }

            override fun onPageFinished(view: WebView?, url: String?) {
                super.onPageFinished(view, url)
                if (loaded.getAndSet(true)) return

                runOnIoDispatcher {
                    // Process nekoInit
                    var ret = ""
                    try {
                        ret = nekoInit()
                        val obj = JSONObject(ret)
                        if (!obj.getBoolean("ok")) {
                            throw Exception("plugin refuse to run: ${obj.optString("reason")}")
                        }
                        val min = obj.getInt("minVersion")
                        if (min > NekoPluginManager.managerVersion) {
                            throw Exception("manager version ${NekoPluginManager.managerVersion} too old, this plugin requires >= $min")
                        }
                        plgConfig = obj
                        NekoPluginManager.updatePlgConfig(plgId, obj)
                        it.resume(obj)
                    } catch (e: Exception) {
                        val e2 = Exception("nekoInit: " + e.readableMessage + "\n\n" + ret)
                        plgConfigException = e2
                        it.resumeWithException(e2)
                    }
                }
            }
        }
        webView!!.loadUrl("http://$plgId/plugin.html")
    }

    // Android call JS

    private suspend fun callJS(script: String): String = suspendCoroutine {
        val jsLatch = CountDownLatch(1)
        var jsReceivedValue = ""

        runOnMainDispatcher {
            if (webView != null) {
                webView!!.evaluateJavascript(script) { value ->
                    jsReceivedValue = value
                    jsLatch.countDown()
                }
            } else {
                jsReceivedValue = "webView is null"
                jsLatch.countDown()
            }
        }

        jsLatch.await(5, TimeUnit.SECONDS)

        // evaluateJavascript escapes Javascript's String
        jsReceivedValue = JavaUtil.unescapeString(jsReceivedValue.removeSurrounding("\""))
        if (BuildConfig.DEBUG) Logs.d("$script: $jsReceivedValue")
        it.resume(jsReceivedValue)
    }

    // call once
    private suspend fun nekoInit(): String {
        val sendData = JSONObject()
        sendData.put("lang", Locale.getDefault().toLanguageTag())
        sendData.put("plgId", plgId)
        sendData.put("managerVersion", NekoPluginManager.managerVersion)

        return callJS(
            "nekoInit(\"${
                Util.b64EncodeUrlSafe(
                    sendData.toString().toByteArray()
                )
            }\")"
        )
    }

    fun switchProtocol(id: String): NekoProtocol {
        lateinit var p: NekoProtocol
        if (protocols.containsKey(id)) {
            p = protocols[id]!!
        } else {
            p = NekoProtocol(id) { callJS(it) }
            protocols[id] = p
        }
        jsObject.protocol = p
        return p
    }

    suspend fun getAbout(): String {
        return callJS("nekoAbout()")
    }

    inner class NekoProtocol(val protocolId: String, val callJS: suspend (String) -> String) {
        private suspend fun callProtocol(method: String, b64Str: String?): String {
            var arg = ""
            if (b64Str != null) {
                arg = "\"" + b64Str + "\""
            }
            return callJS("nekoProtocol(\"$protocolId\").$method($arg)")
        }

        suspend fun buildAllConfig(
            port: Int, bean: NekoBean, otherArgs: Map<String, Any>?
        ): String {
            val sendData = JSONObject()
            sendData.put("port", port)
            sendData.put(
                "sharedStorage",
                Util.b64EncodeUrlSafe(bean.sharedStorage.toString().toByteArray())
            )
            otherArgs?.forEach { (t, u) -> sendData.put(t, u) }

            return callProtocol(
                "buildAllConfig", Util.b64EncodeUrlSafe(sendData.toString().toByteArray())
            )
        }

        suspend fun parseShareLink(shareLink: String): String {
            val sendData = JSONObject()
            sendData.put("shareLink", shareLink)

            return callProtocol(
                "parseShareLink", Util.b64EncodeUrlSafe(sendData.toString().toByteArray())
            )
        }

        // UI Interface

        suspend fun setSharedStorage(sharedStorage: String) {
            callProtocol(
                "setSharedStorage",
                Util.b64EncodeUrlSafe(sharedStorage.toByteArray())
            )
        }

        suspend fun requireSetProfileCache() {
            callProtocol("requireSetProfileCache", null)
        }

        suspend fun requirePreferenceScreenConfig(): String {
            return callProtocol("requirePreferenceScreenConfig", null)
        }

        suspend fun sharedStorageFromProfileCache(): String {
            return callProtocol("sharedStorageFromProfileCache", null)
        }

        suspend fun onPreferenceCreated() {
            callProtocol("onPreferenceCreated", null)
        }

        suspend fun onPreferenceChanged(key: String, v: Any) {
            val sendData = JSONObject()
            sendData.put("key", key)
            sendData.put("newValue", v)

            callProtocol(
                "onPreferenceChanged",
                Util.b64EncodeUrlSafe(sendData.toString().toByteArray())
            )
        }

    }

    inner class JsObject {
        var preferenceScreen: PreferenceScreen? = null
        var protocol: NekoProtocol? = null

        // JS call Android

        @JavascriptInterface
        fun toast(s: String) {
            Toast.makeText(FeederApplication.instance.applicationContext, s, Toast.LENGTH_SHORT).show()
        }

        @JavascriptInterface
        fun logError(s: String) {
            Logs.e("logError: $s")
        }

        @JavascriptInterface
        fun setPreferenceVisibility(key: String, isVisible: Boolean) {
            runBlockingOnMainDispatcher {
                preferenceScreen?.findPreference<Preference>(key)?.isVisible = isVisible
            }
        }

        @JavascriptInterface
        fun setPreferenceTitle(key: String, title: String) {
            runBlockingOnMainDispatcher {
                preferenceScreen?.findPreference<Preference>(key)?.title = title
            }
        }

        @JavascriptInterface
        fun listenOnPreferenceChanged(key: String) {
            preferenceScreen?.findPreference<Preference>(key)
                ?.setOnPreferenceChangeListener { preference, newValue ->
                    runOnIoDispatcher {
                        protocol?.onPreferenceChanged(preference.key, newValue)
                    }
                    true
                }
        }

        @JavascriptInterface
        fun setKV(type: Int, key: String, jsonStr: String) {
            try {
                val v = JSONObject(jsonStr)
                when (type) {
                    0 -> DataStore.profileCacheStore.putBoolean(key, v.getBoolean("v"))
                    1 -> DataStore.profileCacheStore.putFloat(key, v.getDouble("v").toFloat())
                    2 -> DataStore.profileCacheStore.putInt(key, v.getInt("v"))
                    3 -> DataStore.profileCacheStore.putLong(key, v.getLong("v"))
                    4 -> DataStore.profileCacheStore.putString(key, v.getString("v"))
                }
            } catch (e: Exception) {
                Logs.e("setKV: $e")
            }
        }

        @JavascriptInterface
        fun getKV(type: Int, key: String): String {
            val v = JSONObject()
            try {
                when (type) {
                    0 -> v.put("v", DataStore.profileCacheStore.getBoolean(key))
                    1 -> v.put("v", DataStore.profileCacheStore.getFloat(key))
                    2 -> v.put("v", DataStore.profileCacheStore.getInt(key))
                    3 -> v.put("v", DataStore.profileCacheStore.getLong(key))
                    4 -> v.put("v", DataStore.profileCacheStore.getString(key))
                }
            } catch (e: Exception) {
                Logs.e("getKV: $e")
            }
            return v.toString()
        }

    }

    fun destroy() {
        webView?.onPause()
        webView?.removeAllViews()
        webView?.destroy()
        webView = null
    }

    suspend fun destorySuspend() = withContext(Dispatchers.Main) {
        destroy()
    }

    object Default {
        val map = mutableMapOf<String, NekoJSInterface>()

        suspend fun destroyJsi(plgId: String) = withContext(Dispatchers.Main) {
            if (map.containsKey(plgId)) {
                map[plgId]!!.destroy()
                map.remove(plgId)
            }
        }

        // now it's manually managed
        suspend fun destroyAllJsi() = withContext(Dispatchers.Main) {
            map.forEach { (t, u) ->
                u.destroy()
                map.remove(t)
            }
        }

        suspend fun requireJsi(plgId: String): NekoJSInterface = withContext(Dispatchers.Main) {
            lateinit var jsi: NekoJSInterface
            if (map.containsKey(plgId)) {
                jsi = map[plgId]!!
            } else {
                jsi = NekoJSInterface(plgId)
                map[plgId] = jsi
            }
            return@withContext jsi
        }
    }
}
