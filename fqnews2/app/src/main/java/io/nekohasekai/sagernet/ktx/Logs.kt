package io.nekohasekai.sagernet.ktx

import libcore.Libcore
import java.io.InputStream
import java.io.OutputStream

object Logs {

    private fun mkTag(): String {
        val stackTrace = Thread.currentThread().stackTrace
        return stackTrace[4].className.substringAfterLast(".")
    }

    // level int use logrus.go

    fun d(message: String) {
        Libcore.nekoLogPrintln("[Debug] [${mkTag()}] $message")
    }

    fun d(message: String, exception: Throwable) {
        Libcore.nekoLogPrintln("[Debug] [${mkTag()}] $message" + "\n" + exception.stackTraceToString())
    }

    fun i(message: String) {
        Libcore.nekoLogPrintln("[Info] [${mkTag()}] $message")
    }

    fun i(message: String, exception: Throwable) {
        Libcore.nekoLogPrintln("[Info] [${mkTag()}] $message" + "\n" + exception.stackTraceToString())
    }

    fun w(message: String) {
        Libcore.nekoLogPrintln("[Warning] [${mkTag()}] $message")
    }

    fun w(message: String, exception: Throwable) {
        Libcore.nekoLogPrintln("[Warning] [${mkTag()}] $message" + "\n" + exception.stackTraceToString())
    }

    fun w(exception: Throwable) {
        Libcore.nekoLogPrintln("[Warning] [${mkTag()}] " + exception.stackTraceToString())
    }

    fun e(message: String) {
        Libcore.nekoLogPrintln("[Error] [${mkTag()}] $message")
    }

    fun e(message: String, exception: Throwable) {
        Libcore.nekoLogPrintln("[Error] [${mkTag()}] $message" + "\n" + exception.stackTraceToString())
    }

    fun e(exception: Throwable) {
        Libcore.nekoLogPrintln("[Error] [${mkTag()}] " + exception.stackTraceToString())
    }

}

fun InputStream.use(out: OutputStream) {
    use { input ->
        out.use { output ->
            input.copyTo(output)
        }
    }
}