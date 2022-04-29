/*******************************************************************************
 *                                                                             *
 *  Copyright (C) 2018 by Max Lv <max.c.lv@gmail.com>                          *
 *  Copyright (C) 2018 by Mygod Studio <contact-shadowsocks-android@mygod.be>  *
 *                                                                             *
 *  This program is free software: you can redistribute it and/or modify       *
 *  it under the terms of the GNU General Public License as published by       *
 *  the Free Software Foundation, either version 3 of the License, or          *
 *  (at your option) any later version.                                        *
 *                                                                             *
 *  This program is distributed in the hope that it will be useful,            *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 *  GNU General Public License for more details.                               *
 *                                                                             *
 *  You should have received a copy of the GNU General Public License          *
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

package com.github.shadowsocks.utils

import android.annotation.SuppressLint
import android.app.Application
import android.content.*
import android.content.pm.PackageInfo
import android.content.res.Resources
import android.graphics.BitmapFactory
import android.graphics.ImageDecoder
import android.net.Uri
import android.os.Build
import android.system.ErrnoException
import android.system.Os
import android.system.OsConstants
import android.util.Base64
import android.util.Log
import android.util.TypedValue
import androidx.annotation.AttrRes
import androidx.preference.Preference
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import kotlinx.coroutines.suspendCancellableCoroutine
import java.io.FileDescriptor
import java.net.HttpURLConnection
import java.net.InetAddress
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException

fun <T> Iterable<T>.forEachTry(action: (T) -> Unit) {
    var result: Exception? = null
    for (element in this) try {
        action(element)
    } catch (e: Exception) {
        if (result == null) result = e else result.addSuppressed(e)
    }
    if (result != null) {
        result.printStackTrace()
        throw result
    }
}

val Throwable.readableMessage get() = localizedMessage ?: javaClass.name

/**
 * https://android.googlesource.com/platform/prebuilts/runtime/+/94fec32/appcompat/hiddenapi-light-greylist.txt#9466
 */
private val getInt = FileDescriptor::class.java.getDeclaredMethod("getInt$")
val FileDescriptor.int get() = getInt.invoke(this) as Int

fun FileDescriptor.closeQuietly() = try {
    Os.close(this)
} catch (_: ErrnoException) { }

private val parseNumericAddress by lazy @SuppressLint("DiscouragedPrivateApi") {
    InetAddress::class.java.getDeclaredMethod("parseNumericAddress", String::class.java).apply {
        isAccessible = true
    }
}
/**
 * A slightly more performant variant of parseNumericAddress.
 *
 * Bug in Android 9.0 and lower: https://issuetracker.google.com/issues/123456213
 */
fun String?.parseNumericAddress(): InetAddress? = Os.inet_pton(OsConstants.AF_INET, this)
        ?: Os.inet_pton(OsConstants.AF_INET6, this)?.let {
            if (Build.VERSION.SDK_INT >= 29) it else parseNumericAddress.invoke(null, this) as InetAddress
        }

suspend fun <T> HttpURLConnection.useCancellable(block: suspend HttpURLConnection.() -> T): T {
    return suspendCancellableCoroutine { cont ->
        cont.invokeOnCancellation {
            if (Build.VERSION.SDK_INT >= 26) disconnect() else GlobalScope.launch(Dispatchers.IO) { disconnect() }
        }
        GlobalScope.launch(Dispatchers.IO) {
            try {
                cont.resume(block())
            } catch (e: Throwable) {
                cont.resumeWithException(e)
            }
        }
    }
}

fun parsePort(str: String?, default: Int, min: Int = 1025): Int {
    val value = str?.toIntOrNull() ?: default
    return if (value < min || value > 65535) default else value
}

fun broadcastReceiver(callback: (Context, Intent) -> Unit): BroadcastReceiver = object : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) = callback(context, intent)
}

fun Context.listenForPackageChanges(onetime: Boolean = true, callback: () -> Unit) = object : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        callback()
        if (onetime) context.unregisterReceiver(this)
    }
}.apply {
    registerReceiver(this, IntentFilter().apply {
        addAction(Intent.ACTION_PACKAGE_ADDED)
        addAction(Intent.ACTION_PACKAGE_REMOVED)
        addDataScheme("package")
    })
}

fun ContentResolver.openBitmap(uri: Uri) =
        if (Build.VERSION.SDK_INT >= 28) ImageDecoder.decodeBitmap(ImageDecoder.createSource(this, uri))
        else BitmapFactory.decodeStream(openInputStream(uri))

val PackageInfo.signaturesCompat get() =
    if (Build.VERSION.SDK_INT >= 28) signingInfo.apkContentsSigners else @Suppress("DEPRECATION") signatures

/**
 * Based on: https://stackoverflow.com/a/26348729/2245107
 */
fun Resources.Theme.resolveResourceId(@AttrRes resId: Int): Int {
    val typedValue = TypedValue()
    if (!resolveAttribute(resId, typedValue, true)) throw Resources.NotFoundException()
    return typedValue.resourceId
}

val Intent.datas get() = listOfNotNull(data) + (clipData?.asIterable()?.mapNotNull { it.uri } ?: emptyList())

fun printLog(t: Throwable) {
    //Crashlytics.logException(t)
    Log.e("Utils","printLog",t)
    t.printStackTrace()
}
fun printLog(t: String?) {
    if(t==null)return
    //Crashlytics.logException(t)
    Log.e("Utils",t)
}
fun printLog(logLever:Int,tag:String,s: String?) {
    if(s.isNullOrEmpty())return;
    if (logLever==Log.WARN)
        Log.w(tag,s)
    if (logLever==Log.ERROR)
        Log.e(tag,s)

}
fun Preference.remove() = parent!!.removePreference(this)

/**
 * parseInt
 */
fun parseInt(str: String): Int {
    try {
        return Integer.parseInt(str)
    } catch (e: Exception) {
        e.printStackTrace()
        return 0
    }
}

/**
 * package path
 */
fun packagePath(context: Context): String {
    var path = context.filesDir.toString()
    path = path.replace("files", "")
    //path += "tun2socks"

    return path
}

fun isIpv6Address(value: String): Boolean {
    var addr = value
    if (addr.indexOf("[") == 0 && addr.lastIndexOf("]") > 0) {
        addr = addr.drop(1)
        addr = addr.dropLast(addr.count() - addr.lastIndexOf("]"))
    }
    val regV6 = Regex("^((?:[0-9A-Fa-f]{1,4}))?((?::[0-9A-Fa-f]{1,4}))*::((?:[0-9A-Fa-f]{1,4}))?((?::[0-9A-Fa-f]{1,4}))*|((?:[0-9A-Fa-f]{1,4}))((?::[0-9A-Fa-f]{1,4})){7}$")
    return regV6.matches(addr)
}

/**
 * readTextFromAssets
 */
fun readTextFromAssets(app: Application, fileName: String): String {
    val content = app.assets.open(fileName).bufferedReader().use {
        it.readText()
    }
    return content
}

/**
 * is ip address
 */
fun isIpAddress(value: String): Boolean {
    try {
        var addr = value
        if (addr.isEmpty() || addr.isBlank()) {
            return false
        }
        //CIDR
        if (addr.indexOf("/") > 0) {
            val arr = addr.split("/")
            if (arr.count() == 2 && Integer.parseInt(arr[1]) > 0) {
                addr = arr[0]
            }
        }

        // "::ffff:192.168.173.22"
        // "[::ffff:192.168.173.22]:80"
        if (addr.startsWith("::ffff:") && '.' in addr) {
            addr = addr.drop(7)
        } else if (addr.startsWith("[::ffff:") && '.' in addr) {
            addr = addr.drop(8).replace("]", "")
        }

        // addr = addr.toLowerCase()
        var octets = addr.split('.').toTypedArray()
        if (octets.size == 4) {
            if(octets[3].indexOf(":") > 0) {
                addr = addr.substring(0, addr.indexOf(":"))
            }
            return isIpv4Address(addr)
        }

        // Ipv6addr [2001:abc::123]:8080
        return isIpv6Address(addr)
    } catch (e: Exception) {
        e.printStackTrace()
        return false
    }
}

fun isIpv4Address(value: String): Boolean {
    val regV4 = Regex("^([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])\\.([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])\\.([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])\\.([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])$")
    return regV4.matches(value)
}

fun Long.toSpeedString() = toTrafficString() + "/s"
const val threshold = 1000
const val divisor = 1024F
fun Long.toTrafficString(): String {
    if (this < threshold)
        return "$this B"

    val kib = this / divisor
    if (kib < threshold)
        return "${kib.toShortString()} KB"

    val mib = kib / divisor
    if (mib < threshold)
        return "${mib.toShortString()} MB"

    val gib = mib / divisor
    if (gib < threshold)
        return "${gib.toShortString()} GB"

    val tib = gib / divisor
    if (tib < threshold)
        return "${tib.toShortString()} TB"

    val pib = tib / divisor
    if (pib < threshold)
        return "${pib.toShortString()} PB"

    return "∞"
}

private fun Float.toShortString(): String {
    val s = toString()
    if (s.length <= 4)
        return s
    return s.substring(0, 4).removeSuffix(".")
}

/**
 * base64 encode
 */
fun encodeForVmess(text: String): String {
    try {
        return Base64.encodeToString(text.toByteArray(charset("UTF-8")), Base64.NO_WRAP)
    } catch (e: Exception) {
        e.printStackTrace()
        return ""
    }
}