package io.nekohasekai.sagernet.utils

import android.annotation.SuppressLint
import android.content.Intent
import android.os.Build
import android.util.Log
import com.jakewharton.processphoenix.ProcessPhoenix
import com.nononsenseapps.feeder.BuildConfig
import io.nekohasekai.sagernet.database.preference.PublicDatabase
import io.nekohasekai.sagernet.ktx.Logs
import io.nekohasekai.sagernet.ktx.app
import io.nekohasekai.sagernet.ui.BlankActivity
import java.io.BufferedReader
import java.io.IOException
import java.io.InputStreamReader
import java.text.SimpleDateFormat
import java.util.*
import java.util.regex.Pattern

object CrashHandler : Thread.UncaughtExceptionHandler {

    @Suppress("UNNECESSARY_SAFE_CALL")
    override fun uncaughtException(thread: Thread, throwable: Throwable) {
        // note: libc / go panic is in android log

        try {
            Log.e(thread.toString(), throwable.stackTraceToString())
        } catch (e: Exception) {
        }

        try {
            Logs.e(thread.toString())
            Logs.e(throwable.stackTraceToString())
        } catch (e: Exception) {
        }

        ProcessPhoenix.triggerRebirth(app, Intent(app, BlankActivity::class.java).apply {
            putExtra("sendLog", "NB4A Crash")
        })
    }

    fun formatThrowable(throwable: Throwable): String {
        var format = throwable.javaClass.name
        val message = throwable.message
        if (!message.isNullOrBlank()) {
            format += ": $message"
        }
        format += "\n"

        format += throwable.stackTrace.joinToString("\n") {
            "    at ${it.className}.${it.methodName}(${it.fileName}:${if (it.isNativeMethod) "native" else it.lineNumber})"
        }

        val cause = throwable.cause
        if (cause != null) {
            format += "\n\nCaused by: " + formatThrowable(cause)
        }

        return format
    }

    fun buildReportHeader(): String {
        var report = ""
        report += "NekoBox for Andoird ${BuildConfig.VERSION_NAME} (${BuildConfig.VERSION_CODE}) ${BuildConfig.FLAVOR.uppercase()}\n"
        report += "Date: ${getCurrentMilliSecondUTCTimeStamp()}\n\n"
        report += "OS_VERSION: ${getSystemPropertyWithAndroidAPI("os.version")}\n"
        report += "SDK_INT: ${Build.VERSION.SDK_INT}\n"
        report += if ("REL" == Build.VERSION.CODENAME) {
            "RELEASE: ${Build.VERSION.RELEASE}"
        } else {
            "CODENAME: ${Build.VERSION.CODENAME}"
        } + "\n"
        report += "ID: ${Build.ID}\n"
        report += "DISPLAY: ${Build.DISPLAY}\n"
        report += "INCREMENTAL: ${Build.VERSION.INCREMENTAL}\n"

        val systemProperties = getSystemProperties()

        report += "SECURITY_PATCH: ${systemProperties.getProperty("ro.build.version.security_patch")}\n"
        report += "IS_DEBUGGABLE: ${systemProperties.getProperty("ro.debuggable")}\n"
        report += "IS_EMULATOR: ${systemProperties.getProperty("ro.boot.qemu")}\n"
        report += "IS_TREBLE_ENABLED: ${systemProperties.getProperty("ro.treble.enabled")}\n"

        report += "TYPE: ${Build.TYPE}\n"
        report += "TAGS: ${Build.TAGS}\n\n"

        report += "MANUFACTURER: ${Build.MANUFACTURER}\n"
        report += "BRAND: ${Build.BRAND}\n"
        report += "MODEL: ${Build.MODEL}\n"
        report += "PRODUCT: ${Build.PRODUCT}\n"
        report += "BOARD: ${Build.BOARD}\n"
        report += "HARDWARE: ${Build.HARDWARE}\n"
        report += "DEVICE: ${Build.DEVICE}\n"
        report += "SUPPORTED_ABIS: ${
            Build.SUPPORTED_ABIS.filter { it.isNotBlank() }.joinToString(", ")
        }\n\n"


        try {
            report += "Settings: \n"
            for (pair in PublicDatabase.kvPairDao.all()) {
                report += "\n"
                report += pair.key + ": " + pair.toString()
            }
        }catch (e: Exception) {
            report += "Export settings failed: " + formatThrowable(e)
        }

        report += "\n\n"

        return report
    }

    private fun getSystemProperties(): Properties {
        val systemProperties = Properties()

        // getprop commands returns values in the format `[key]: [value]`
        // Regex matches string starting with a literal `[`,
        // followed by one or more characters that do not match a closing square bracket as the key,
        // followed by a literal `]: [`,
        // followed by one or more characters as the value,
        // followed by string ending with literal `]`
        // multiline values will be ignored
        val propertiesPattern = Pattern.compile("^\\[([^]]+)]: \\[(.+)]$")
        try {
            val process = ProcessBuilder().command("/system/bin/getprop")
                .redirectErrorStream(true)
                .start()
            val inputStream = process.inputStream
            val bufferedReader = BufferedReader(InputStreamReader(inputStream))
            var line: String?
            var key: String
            var value: String
            while (bufferedReader.readLine().also { line = it } != null) {
                val matcher = propertiesPattern.matcher(line)
                if (matcher.matches()) {
                    key = matcher.group(1)
                    value = matcher.group(2)
                    if (key != null && value != null && !key.isEmpty() && !value.isEmpty()) systemProperties[key] = value
                }
            }
            bufferedReader.close()
            process.destroy()
        } catch (e: IOException) {
            Logs.e(
                "Failed to get run \"/system/bin/getprop\" to get system properties.", e
            )
        }

        //for (String key : systemProperties.stringPropertyNames()) {
        //    Logger.logVerbose(key + ": " +  systemProperties.get(key));
        //}
        return systemProperties
    }

    private fun getSystemPropertyWithAndroidAPI(property: String): String? {
        return try {
            System.getProperty(property)
        } catch (e: Exception) {
            Logs.e("Failed to get system property \"" + property + "\":" + e.message)
            null
        }
    }

    @SuppressLint("SimpleDateFormat")
    private fun getCurrentMilliSecondUTCTimeStamp(): String {
        val df = SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS z")
        df.timeZone = TimeZone.getTimeZone("UTC")
        return df.format(Date())
    }

}