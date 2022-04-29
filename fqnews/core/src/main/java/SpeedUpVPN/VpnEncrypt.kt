package SpeedUpVPN

import SpeedUpVPN.VpnEncrypt.getUniqueID
import android.os.Build
import java.io.File
import java.util.*
import javax.crypto.Cipher
import javax.crypto.spec.IvParameterSpec
import javax.crypto.spec.SecretKeySpec


object VpnEncrypt{
    private const val theKey="theKey"
    const val vpnGroupName="SpeedUp.VPN"
    const val v2vpnRemark="v2ray.vpn"
    const val freesubGroupName="https://git.io/jmsfq"
    const val testing="test..."
    const val ACTION_PROXY_START_COMPLETE = "SpeedUpVPN.ACTION_PROXY_START_COMPLETE"
    const val ACTION_INTERNET_FAIL = "SpeedUpVPN.ACTION_INTERNET_FAIL"
    const val SOCK_PROXY_PORT = 10808
    const val HTTP_PROXY_PORT = 58300
    const val enableLocalDns =  false
    const val enableSniffing = true
    const val PREF_ROUTING_DOMAIN_STRATEGY = "IPIfNonMatch"
    const val enableSpeed = false
    @JvmStatic fun aesEncrypt(v:String, secretKey:String=theKey) = AES256.encrypt(v, secretKey)
    @JvmStatic fun aesDecrypt(v:String, secretKey:String=theKey) = AES256.decrypt(v, secretKey)
    @JvmStatic fun readFileAsTextUsingInputStream(fileName: String)  = File(fileName).inputStream().readBytes().toString(Charsets.UTF_8)
    @JvmStatic fun getUniqueID(): Int {
        var szDevIDShort = "168169"
        try {
            szDevIDShort+=Build.BOARD.length % 10 + Build.BRAND.length % 10 + Build.DEVICE.length % 10 + Build.MANUFACTURER.length % 10 + Build.MODEL.length % 10 + Build.PRODUCT.length % 10
        } catch (exception: Exception) {exception.printStackTrace()}
        var serial: String? = null
        try {
            serial = Build::class.java.getField("SERIAL")[null].toString()
            // Go ahead and return the serial for api => 9
            return UUID(szDevIDShort.hashCode().toLong(), serial.hashCode().toLong()).hashCode()
        } catch (exception: Exception) { // String needs to be initialized
            exception.printStackTrace()
            serial = "https://git.io/jww" // some value
        }

        return UUID(szDevIDShort.hashCode().toLong(), serial.hashCode().toLong()).hashCode()
    }
}

private object AES256{
    private fun cipher(opmode:Int, secretKey:String):Cipher{
        if(secretKey.length != 32) throw RuntimeException("SecretKey length is not 32 chars")
        val c = Cipher.getInstance("AES/CBC/PKCS5Padding")
        val sk = SecretKeySpec(secretKey.toByteArray(Charsets.UTF_8), "AES")
        val iv = IvParameterSpec(secretKey.substring(0, 16).toByteArray(Charsets.UTF_8))
        c.init(opmode, sk, iv)
        return c
    }
    fun encrypt(str:String, secretKey:String):String{
        val encrypted = cipher(Cipher.ENCRYPT_MODE, secretKey).doFinal(str.toByteArray(Charsets.UTF_8))
        var encstr: String
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O || isWindows())
            encstr = java.util.Base64.getEncoder().encodeToString(encrypted)
        else
            encstr = android.util.Base64.encodeToString(encrypted, android.util.Base64.DEFAULT)

        return encstr
    }
    fun decrypt(str:String, secretKey:String):String{
        val byteStr : ByteArray
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O || isWindows())
            byteStr=java.util.Base64.getDecoder().decode(str.toByteArray(Charsets.UTF_8))
        else
            byteStr=android.util.Base64.decode(str.toByteArray(Charsets.UTF_8), android.util.Base64.DEFAULT)

        return String(cipher(Cipher.DECRYPT_MODE, secretKey).doFinal(byteStr))
    }
    fun isWindows(): Boolean {
        var os= System.getProperty("os.name")
        if(os.isNullOrEmpty())
            return false
        else
            return os.contains("Windows")
    }
}

