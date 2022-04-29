/*******************************************************************************
 *                                                                             *
 *  Copyright (C) 2017 by Max Lv <max.c.lv@gmail.com>                          *
 *  Copyright (C) 2017 by Mygod Studio <contact-shadowsocks-android@mygod.be>  *
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

package com.github.shadowsocks.database

import SpeedUpVPN.VpnEncrypt
import android.annotation.TargetApi
import android.net.Uri
import android.os.Parcelable
import android.text.TextUtils
import android.util.Base64
import android.util.Log
import android.util.LongSparseArray
import androidx.core.net.toUri
import androidx.room.*
import com.github.shadowsocks.Core
import com.github.shadowsocks.plugin.PluginConfiguration
import com.github.shadowsocks.plugin.PluginOptions
import com.github.shadowsocks.preference.DataStore
import com.github.shadowsocks.utils.Key
import com.github.shadowsocks.utils.parsePort
import com.google.gson.*
import kotlinx.android.parcel.Parcelize
import org.json.JSONArray
import org.json.JSONObject
import java.io.Serializable
import java.net.URI
import java.net.URISyntaxException
import java.util.*
import com.github.shadowsocks.utils.*
import java.net.URLEncoder


@Entity
@Parcelize
data class Profile(
        @PrimaryKey(autoGenerate = true)
        var id: Long = 0,

        // user configurable fields
        var name: String? = "",

        var host: String = sponsored,
        var remotePort: Int = 8388,
        var password: String = "",
        var method: String = "aes-256-cfb",

        var remoteDns: String = "1.1.1.1",
        var proxyApps: Boolean = false,
        var bypass: Boolean = false,
        var udpdns: Boolean = false,
        var url_group: String = "",
        var ipv6: Boolean = false,
        @TargetApi(28)
        var metered: Boolean = false,
        var individual: String = "",
        var plugin: String? = null,
        var udpFallback: Long? = null,

        // managed fields
        var subscription: SubscriptionStatus = SubscriptionStatus.UserConfigured,
        var tx: Long = 0,
        var rx: Long = 0,
        var elapsed: Long = 0,
        var userOrder: Long = 0,

        var profileType: String=AppConfig.EConfigType.vless,   // vmess|shadowsocks|vless
        var route: String = "all",
        //for v2ray
        var alterId: Int = 64,
        var network: String = "tcp",  //tcp,kcp,ws,h2,quic,grpc
        var headerType: String = "",  //伪装类型（type）
        var requestHost: String = "", //伪装域名（host）
        var path: String = "",        // ws path, h2 path , quic加密密钥

        var xtlsflow: String="", //for vless xtls only

        var streamSecurity: String = "",  //底层传输安全 tls、 "xtls" 或 ""
        var allowInsecure: String = "false", //跳过证书验证
        var SNI: String ="",

        @Ignore // not persisted in db, only used by direct boot
        var dirty: Boolean = false
) : Parcelable, Serializable {
    enum class SubscriptionStatus(val persistedValue: Int) {
        UserConfigured(0),
        Active(1),
        /**
         * This profile is no longer present in subscriptions.
         */
        Obsolete(2),
        ;

        companion object {
            @JvmStatic
            @TypeConverter
            fun of(value: Int) = values().single { it.persistedValue == value }
            @JvmStatic
            @TypeConverter
            fun toInt(status: SubscriptionStatus) = status.persistedValue
        }
    }

    companion object {
        const val VMESS_PROTOCOL: String = "vmess://"
        const val VLESS_PROTOCOL: String = "vless://"
        private const val TAG = "ssvpnParser"
        private const val serialVersionUID = 1L
        private const val sponsored = "198.199.101.152"
        private val pattern =
                """(?m)^(?i)ss://[-a-zA-Z0-9+&@#/%?=.~*'()|!:,;\[\]]*[-a-zA-Z0-9+&@#/%=.~*'()|\[\]]""".toRegex()
        private val userInfoPattern = "^(.+?):(.*)$".toRegex()
        private val legacyPattern = "^(.+?):(.*)@(.+?):(\\d+?)$".toRegex()

        private val pattern_vless = "(?m)^(?i)vless://(.*)".toRegex()
        private val pattern_ssr = "(?m)^(?i)ssr://([A-Za-z0-9_=-]+)".toRegex()
        private val decodedPattern_ssr = "(?i)^((.+):(\\d+?):(.*):(.+):(.*):(.+)/(.*))".toRegex()
        private val decodedPattern_ssr_obfsparam = "(?i)(.*)[?&]obfsparam=([A-Za-z0-9_=-]*)(.*)".toRegex()
        private val decodedPattern_ssr_remarks = "(?i)(.*)[?&]remarks=([A-Za-z0-9_=-]*)(.*)".toRegex()
        private val decodedPattern_ssr_protocolparam = "(?i)(.*)[?&]protoparam=([A-Za-z0-9_=-]*)(.*)".toRegex()
        private val decodedPattern_ssr_groupparam = "(?i)(.*)[?&]group=([A-Za-z0-9_=-]*)(.*)".toRegex()

        private val pattern_vmess = "(?m)^(?i)vmess://(.*)".toRegex()

        private fun base64Decode(data: String) = String(Base64.decode(data.replace("=", ""), Base64.URL_SAFE), Charsets.UTF_8)
        /**
         * base64 decode
         */
        fun decodeForVmess(text: String): String {
            try {
                return Base64.decode(text, Base64.NO_WRAP).toString(charset("UTF-8"))
            } catch (e: Exception) {
                e.printStackTrace()
                return ""
            }
        }

        fun profileFromVmessBean(vmess: VmessBean, profile:Profile): Profile {
            profile.profileType = vmess.configType
            profile.remoteDns=vmess.remoteDns
            profile.host=vmess.address
            profile.alterId=vmess.alterId
            profile.headerType=vmess.headerType
            profile.password=vmess.id
            profile.network=vmess.network
            profile.path=vmess.path
            profile.remotePort=vmess.port
            profile.name=vmess.remarks
            profile.requestHost=vmess.requestHost
            profile.method=vmess.security
            profile.streamSecurity=vmess.streamSecurity
            profile.allowInsecure=vmess.allowInsecure
            profile.SNI=vmess.SNI
            profile.xtlsflow=vmess.flow
            profile.url_group=vmess.subid

            if(vmess.route=="0")profile.route="all"
            else if(vmess.route=="1")profile.route="bypass-lan"
            else if(vmess.route=="2")profile.route="bypass-china"
            else if(vmess.route=="3")profile.route="bypass-lan-china"
            else profile.route="all"

            return profile
        }


        private fun ResolveVmess4Kitsunebi(server: String): VmessBean {
            val vmess = VmessBean()
            vmess.configType=AppConfig.EConfigType.vmess
            var result = server.replace(VMESS_PROTOCOL, "")
            val indexSplit = result.indexOf("?")
            if (indexSplit > 0) {
                result = result.substring(0, indexSplit)
            }
            result = decodeForVmess(result)

            val arr1 = result.split('@')
            if (arr1.count() != 2) {
                return vmess
            }
            val arr21 = arr1[0].split(':')
            val arr22 = arr1[1].split(':')
            if (arr21.count() != 2 || arr21.count() != 2) {
                return vmess
            }

            vmess.address = arr22[0]
            vmess.port = parseInt(arr22[1])
            vmess.security = arr21[0]
            vmess.id = arr21[1]

            vmess.security = "chacha20-poly1305"
            vmess.network = "tcp"
            vmess.headerType = "none"
            vmess.remarks = "Alien"
            vmess.alterId = 0

            return vmess
        }
        fun findAllVmessUrls(data: CharSequence?, feature: Profile? = null) = pattern_vmess.findAll(data
                ?: "").map {
            val server = it.groupValues[1]
            val profile = Profile()
            feature?.copyFeatureSettingsTo(profile)
            try {
                val indexSplit = server.indexOf("?")
                if (indexSplit > 0) {
                    profileFromVmessBean(ResolveVmess4Kitsunebi(server),profile)
                } else {
                    var result = server.replace(VMESS_PROTOCOL, "")
                    result = decodeForVmess(result)
                    //Log.e("vmess://",result)
                    if (TextUtils.isEmpty(result)) {
                        null
                    }
                    var vmessQRCode = VmessQRCode()
                    vmessQRCode = Gson().fromJson(result, vmessQRCode::class.java)
                    if (TextUtils.isEmpty(vmessQRCode.add)
                            || TextUtils.isEmpty(vmessQRCode.port)
                            || TextUtils.isEmpty(vmessQRCode.id)
                            || TextUtils.isEmpty(vmessQRCode.aid)
                            || TextUtils.isEmpty(vmessQRCode.net)
                    ) {
                        null
                    }
                    profile.profileType= AppConfig.EConfigType.vmess
                    profile.method = "auto"
                    profile.network = "tcp"
                    profile.headerType = "none"
                    profile.host = vmessQRCode.add
                    profile.name = vmessQRCode.ps
                    //if (profile.name==VpnEncrypt.v2vpnRemark)profile.name = profile.host.substring(0,5)+"..."
                    profile.remotePort = parseInt(vmessQRCode.port)
                    profile.password = vmessQRCode.id
                    profile.alterId = parseInt(vmessQRCode.aid)
                    profile.network = vmessQRCode.net
                    profile.headerType = vmessQRCode.type
                    profile.requestHost = vmessQRCode.host
                    profile.path = vmessQRCode.path
                    profile.streamSecurity = vmessQRCode.tls
                    profile.allowInsecure = vmessQRCode.allowInsecure?:"false"
                    profile.SNI = vmessQRCode.sni?:""
                    profile
                }
            } catch (e: Exception) {
                printLog(e)
                Log.e(TAG, "Invalid Vmess URI: ${it.value}")
                null
            }
        }.filterNotNull()

        fun findAllSSRUrls(data: CharSequence?, feature: Profile? = null) = pattern_ssr.findAll(data
                ?: "").map {
            val uri = base64Decode(it.groupValues[1])
            try {
                val match = decodedPattern_ssr.matchEntire(uri)
                if (match != null) {
                    val profile = Profile()
                    feature?.copyFeatureSettingsTo(profile)
                    profile.profileType= AppConfig.EConfigType.shadowsocks
                    profile.host = match.groupValues[2].toLowerCase(Locale.ENGLISH)
                    profile.remotePort = match.groupValues[3].toInt()
                    profile.method = match.groupValues[5].toLowerCase(Locale.ENGLISH)
                    profile.password = base64Decode(match.groupValues[7])

                    val match1 = decodedPattern_ssr_obfsparam.matchEntire(match.groupValues[8])

                    val match2 = decodedPattern_ssr_protocolparam.matchEntire(match.groupValues[8])

                    val match3 = decodedPattern_ssr_remarks.matchEntire(match.groupValues[8])
                    if (match3 != null) profile.name = base64Decode(match3.groupValues[2])

                    val match4 = decodedPattern_ssr_groupparam.matchEntire(match.groupValues[8])
                    if (match4 != null) profile.url_group = base64Decode(match4.groupValues[2])

                    profile
                } else {
                    null
                }
            } catch (e: IllegalArgumentException) {
                Log.e(TAG, "Invalid SSR URI: ${it.value}")
                null
            }
        }.filterNotNull()

        fun findAllUrls(response: CharSequence?, feature: Profile? = null):MutableSet<Profile> {
            var profilesSet:MutableSet<Profile> = LinkedHashSet<Profile>()
            val ssrProfiles = findAllSSRUrls(response, feature)
            val ssPofiles = findAllSSUrls(response, feature)
            val vmessProfiles= findAllVmessUrls(response, feature)
            val vlessProfiles= findAllVlessUrls(response, feature)
            if(vlessProfiles.count()>0)profilesSet.addAll(vlessProfiles)
            if(vmessProfiles.count()>0)profilesSet.addAll(vmessProfiles)
            if(ssPofiles.count()>0)profilesSet.addAll(ssPofiles)
            if(ssrProfiles.count()>0)profilesSet.addAll(ssrProfiles)
            return profilesSet
        }
        fun findAllVlessUrls(data: CharSequence?, feature: Profile? = null) = pattern_vless.findAll(data ?: "").map {
            try {
                //Log.e("vless----",it.value)
                val uri = it.value.toUri()
                val profile = Profile()
                feature?.copyFeatureSettingsTo(profile)
                profile.profileType = AppConfig.EConfigType.vless
                profile.password = uri.userInfo.toString()

                // bug in Android: https://code.google.com/p/android/issues/detail?id=192855
                val javaURI = URI(it.value)
                profile.host = javaURI.host ?: ""
                if (profile.host.firstOrNull() == '[' && profile.host.lastOrNull() == ']') {
                    profile.host = profile.host.substring(1, profile.host.length - 1)
                }
                profile.remotePort = javaURI.port
                profile.method = uri.getQueryParameter("encryption")?:"none"
                profile.network = uri.getQueryParameter("type")!!
                profile.headerType = uri.getQueryParameter("headerType")?:"none"
                profile.requestHost = uri.getQueryParameter("host")?:""
                profile.path = uri.getQueryParameter("path")?:"/"

                if(profile.network == "h2" && profile.requestHost=="")profile.requestHost=profile.host

                if(profile.network == "quic"){
                    profile.requestHost=uri.getQueryParameter("quicSecurity")?:"none"
                    profile.path = uri.getQueryParameter("key")?:""
                }

                if(profile.network == "kcp")profile.path = uri.getQueryParameter("seed")?:"seed"
                if(profile.network == "tcp")profile.path = ""

                profile.streamSecurity = uri.getQueryParameter("security")?:"tls"
                profile.SNI = uri.getQueryParameter("sni")?:""
                profile.xtlsflow = uri.getQueryParameter("flow")?:""

                if(isIpAddress(profile.host) && profile.SNI=="" && profile.requestHost!="")profile.SNI=profile.requestHost

                profile.name = uri.fragment ?: ""
                profile
            }
            catch (e:Exception){
                Log.e(TAG, "Invalid URI: ${it.value}")
                null
            }
        }.filterNotNull()

        fun findAllSSUrls(data: CharSequence?, feature: Profile? = null) = pattern.findAll(data ?: "").map {
            val uri = it.value.toUri()
            try {
                if (uri.userInfo == null) {
                    val match = legacyPattern.matchEntire(String(Base64.decode(uri.host, Base64.NO_PADDING)))
                    if (match != null) {
                        val profile = Profile()
                        feature?.copyFeatureSettingsTo(profile)
                        profile.profileType= AppConfig.EConfigType.shadowsocks
                        profile.method = match.groupValues[1].toLowerCase(Locale.ENGLISH)
                        profile.password = match.groupValues[2]
                        profile.host = match.groupValues[3]
                        profile.remotePort = match.groupValues[4].toInt()
                        profile.plugin = uri.getQueryParameter(Key.plugin)
                        profile.name = uri.fragment
                        profile
                    } else {
                        Log.w(TAG, "Unrecognized URI")
                        null
                    }
                } else {
                    val match = userInfoPattern.matchEntire(String(Base64.decode(uri.userInfo,
                            Base64.NO_PADDING or Base64.NO_WRAP or Base64.URL_SAFE)))
                    if (match != null) {
                        val profile = Profile()
                        feature?.copyFeatureSettingsTo(profile)
                        profile.profileType= AppConfig.EConfigType.shadowsocks
                        profile.method = match.groupValues[1]
                        profile.password = match.groupValues[2]
                        // bug in Android: https://code.google.com/p/android/issues/detail?id=192855
                        try {
                            val javaURI = URI(it.value)
                            profile.host = javaURI.host ?: ""
                            if (profile.host.firstOrNull() == '[' && profile.host.lastOrNull() == ']') {
                                profile.host = profile.host.substring(1, profile.host.length - 1)
                            }
                            profile.remotePort = javaURI.port
                            profile.plugin = uri.getQueryParameter(Key.plugin)
                            profile.name = uri.fragment ?: ""
                            profile
                        } catch (e: URISyntaxException) {
                            Log.e(TAG, "Invalid URI: ${it.value}")
                            null
                        }
                    } else {
                        Log.e(TAG, "Unknown user info: ${it.value}")
                        null
                    }
                }
            } catch (e: IllegalArgumentException) {
                Log.e(TAG, "Invalid base64 detected: ${it.value}")
                null
            }
        }.filterNotNull()

        private class JsonParser(private val feature: Profile? = null) : ArrayList<Profile>() {
            val fallbackMap = mutableMapOf<Profile, Profile>()

            private val JsonElement?.optString get() = (this as? JsonPrimitive)?.asString
            private val JsonElement?.optBoolean
                get() = // asBoolean attempts to cast everything to boolean
                    (this as? JsonPrimitive)?.run { if (isBoolean) asBoolean else null }
            private val JsonElement?.optInt
                get() = try {
                    (this as? JsonPrimitive)?.asInt
                } catch (_: NumberFormatException) {
                    null
                }

            private fun tryParse(json: JsonObject, fallback: Boolean = false): Profile? {
                val host = json["server"].optString
                if (host.isNullOrEmpty()) return null
                val remotePort = json["server_port"]?.optInt
                if (remotePort == null || remotePort <= 0) return null
                val password = json["password"].optString
                if (password.isNullOrEmpty()) return null
                val method = json["method"].optString
                if (method.isNullOrEmpty()) return null
                return Profile().also {
                    it.host = host
                    it.remotePort = remotePort
                    it.password = password
                    it.method = method
                }.apply {
                    feature?.copyFeatureSettingsTo(this)
                    val id = json["plugin"].optString
                    if (!id.isNullOrEmpty()) {
                        plugin = PluginOptions(id, json["plugin_opts"].optString).toString(false)
                    }
                    name = json["remarks"].optString
                    route = json["route"].optString ?: route
                    if (fallback) return@apply
                    remoteDns = json["remote_dns"].optString ?: remoteDns
                    ipv6 = json["ipv6"].optBoolean ?: ipv6
                    metered = json["metered"].optBoolean ?: metered
                    (json["proxy_apps"] as? JsonObject)?.also {
                        proxyApps = it["enabled"].optBoolean ?: proxyApps
                        bypass = it["bypass"].optBoolean ?: bypass
                        individual = (it["android_list"] as? JsonArray)?.asIterable()?.mapNotNull { it.optString }
                                ?.joinToString("\n") ?: individual
                    }
                    udpdns = json["udpdns"].optBoolean ?: udpdns
                    (json["udp_fallback"] as? JsonObject)?.let { tryParse(it, true) }?.also { fallbackMap[this] = it }
                }
            }

            fun process(json: JsonElement?) {
                when (json) {
                    is JsonObject -> {
                        val profile = tryParse(json)
                        if (profile != null) add(profile) else for ((_, value) in json.entrySet()) process(value)
                    }
                    is JsonArray -> json.asIterable().forEach(this::process)
                    // ignore other types
                }
            }

            fun finalize(create: (Profile) -> Profile) {
                val profiles = ProfileManager.getAllProfiles() ?: emptyList()
                for ((profile, fallback) in fallbackMap) {
                    val match = profiles.firstOrNull {
                        fallback.host == it.host && fallback.remotePort == it.remotePort &&
                                fallback.password == it.password && fallback.method == it.method &&
                                it.plugin.isNullOrEmpty()
                    }
                    profile.udpFallback = (match ?: create(fallback)).id
                    ProfileManager.updateProfile(profile)
                }
            }
        }

        fun parseJson(json: JsonElement, feature: Profile? = null, create: (Profile) -> Profile) {
            JsonParser(feature).run {
                process(json)
                for (i in indices) {
                    val fallback = fallbackMap.remove(this[i])
                    this[i] = create(this[i])
                    fallback?.also { fallbackMap[this[i]] = it }
                }
                finalize(create)
            }
        }
    }

    @androidx.room.Dao
    interface Dao {
        @Query("SELECT * FROM `Profile` WHERE `id` = :id")
        operator fun get(id: Long): Profile?

        @Query("SELECT * FROM `Profile` WHERE `host` = :host LIMIT 1")
        fun getByHost(host: String): Profile?

        @Query("SELECT * FROM `Profile` WHERE `host` = :host and `remotePort` = :port and `path` = :path and `profileType` = :profileType and `streamSecurity` = :streamSecurity and `SNI` = :SNI LIMIT 1")
        fun getSameProfile(host: String,port:Int,path: String,profileType: String,streamSecurity: String,SNI: String): Profile?

        @Query("SELECT * FROM `Profile` WHERE `Subscription` != 2 ORDER BY `userOrder`")
        fun listActive(): List<Profile>

        @Query("SELECT * FROM `Profile`")
        fun listAll(): List<Profile>

        @Query("SELECT * FROM `Profile` ORDER BY `url_group`,`elapsed`")
        fun listAllbySpeed(): List<Profile>

        @Query("SELECT * FROM `Profile` WHERE `url_group` = :group")
        fun listByGroup(group: String): List<Profile>

        @Query("SELECT * FROM `Profile` WHERE `url_group` != :group")
        fun listIgnoreGroup(group: String): List<Profile>

        @Query("SELECT MAX(`userOrder`) + 1 FROM `Profile`")
        fun nextOrder(): Long?

        @Query("SELECT 1 FROM `Profile` LIMIT 1")
        fun isNotEmpty(): Boolean

        @Insert
        fun create(value: Profile): Long

        @Update
        fun update(value: Profile): Int

        @Query("DELETE FROM `Profile` WHERE `id` = :id")
        fun delete(id: Long): Int

        @Query("DELETE FROM `Profile`")
        fun deleteAll(): Int
    }

    val formattedAddress get() = (if (host.contains(":")) "[%s]:%d" else "%s:%d").format(host, remotePort)
    val formattedName get() = if (name.isNullOrEmpty()) formattedAddress else name!!

    fun copyFeatureSettingsTo(profile: Profile) {
        profile.route = route
        profile.ipv6 = ipv6
        profile.metered = metered
        profile.proxyApps = proxyApps
        profile.bypass = bypass
        profile.individual = individual
        profile.udpdns = udpdns
    }

    fun toSsUri(): Uri {
        val auth = Base64.encodeToString("$method:$password".toByteArray(),
                Base64.NO_PADDING or Base64.NO_WRAP or Base64.URL_SAFE)
        val wrappedHost = if (host.contains(':')) "[$host]" else host
        val builder = Uri.Builder()
                .scheme("ss")
                .encodedAuthority("$auth@$wrappedHost:$remotePort")
        val configuration = PluginConfiguration(plugin ?: "")
        if (configuration.selected.isNotEmpty()) {
            builder.appendQueryParameter(Key.plugin, configuration.getOptions().toString(false))
        }
        if (!name.isNullOrEmpty()) builder.fragment(name)
        return builder.build()
    }
    fun toVmessUri(): String {
        if (isBuiltin()) return ""
        try {
            val vmess = ProfileManager.profileToVmessBean(this)
                val vmessQRCode = VmessQRCode()
                vmessQRCode.v = vmess.configVersion.toString()
                vmessQRCode.ps = vmess.remarks
                vmessQRCode.add = vmess.address
                vmessQRCode.port = vmess.port.toString()
                vmessQRCode.id = vmess.id
                vmessQRCode.aid = vmess.alterId.toString()
                vmessQRCode.net = vmess.network
                vmessQRCode.type = vmess.headerType
                vmessQRCode.host = vmess.requestHost
                vmessQRCode.path = vmess.path
                vmessQRCode.tls = vmess.streamSecurity
            vmessQRCode.sni = vmess.SNI
            vmessQRCode.allowInsecure = vmess.allowInsecure

                val json = Gson().toJson(vmessQRCode)
                val conf = VMESS_PROTOCOL + encodeForVmess(json)
                return conf
        } catch (e: Exception) {
            e.printStackTrace()
            return ""
        }
    }
    fun toVlessUri(): String {
        if (isBuiltin()) return ""
        try {
            var conf = VLESS_PROTOCOL + password +"@"+host+":"+remotePort+"?encryption="+method
            conf=conf+"&type="+network

            if(streamSecurity!="")conf=conf+"&security="+streamSecurity
            if(SNI!="")conf=conf+"&sni="+SNI
            if(allowInsecure!="false")conf=conf+"&allowInsecure="+allowInsecure
            if(xtlsflow!="")conf=conf+"&flow="+xtlsflow
            when (network){
                "ws"  ->{
                    if(path!="")conf=conf+"&path="+URLEncoder.encode(path?:"/", "utf-8")
                    if(requestHost!="")conf=conf+"&host="+ URLEncoder.encode(requestHost, "utf-8")
                }
                "h2"  ->{
                    if(path!="")conf=conf+"&path="+URLEncoder.encode(path?:"/", "utf-8")
                    if(requestHost!="")conf=conf+"&host="+ URLEncoder.encode(requestHost, "utf-8")
                }
                "kcp"  ->{
                    if(headerType!="")conf=conf+"&headerType="+headerType
                    if(path!="")conf=conf+"&seed="+ URLEncoder.encode(path, "utf-8")
                }
                "quic"  ->{
                    if(requestHost!="")conf=conf+"&quicSecurity="+requestHost
                    if(headerType!="")conf=conf+"&headerType="+headerType
                    if(requestHost!="" && requestHost!="none" && path!="")conf=conf+"&key="+ URLEncoder.encode(path, "utf-8")
                }
                "tcp"  ->{
                    if(headerType!="")conf=conf+"&headerType="+headerType
                    if(requestHost!="")conf=conf+"&host="+URLEncoder.encode(requestHost, "utf-8")
                }
                "grpc"  ->{

                }
                else  -> {

                }
            }

            if (name!="")conf=conf+"#"+ URLEncoder.encode(name?:"", "utf-8")

            return conf
        } catch (e: Exception) {
            e.printStackTrace()
            return ""
        }
    }
    fun isSameAs(other: Profile): Boolean =
        other.host == host && other.remotePort == remotePort && other.path==path
                && other.profileType==profileType && other.streamSecurity==streamSecurity && other.SNI==SNI

    override fun toString() : String {
        if(profileType=="vmess")
            return toVmessUri()
        else if(profileType=="vless")
            return toVlessUri()
        else if (profileType=="ss" || profileType=="shadowsocks") // v5.1.28 before ss
            return toSsUri().toString()
        else return ""
    }

    fun toJson(profiles: LongSparseArray<Profile>? = null): JSONObject = JSONObject().apply {
        if (profileType=="vmess" || profileType=="vless")return@apply
        put("server", host)
        put("server_port", remotePort)
        put("password", password)
        put("method", method)
        if (profiles == null) return@apply
        PluginConfiguration(plugin ?: "").getOptions().also {
            if (it.id.isNotEmpty()) {
                put("plugin", it.id)
                put("plugin_opts", it.toString())
            }
        }
        put("remarks", name)
        put("route", route)
        put("remote_dns", remoteDns)
        put("ipv6", ipv6)
        put("metered", metered)
        put("proxy_apps", JSONObject().apply {
            put("enabled", proxyApps)
            if (proxyApps) {
                put("bypass", bypass)
                // android_ prefix is used because package names are Android specific
                put("android_list", JSONArray(individual.split("\n")))
            }
        })
        put("udpdns", udpdns)
        val fallback = profiles.get(udpFallback ?: return@apply)
        if (fallback != null && fallback.plugin.isNullOrEmpty()) fallback.toJson().also { put("udp_fallback", it) }
    }

    val isSponsored get() = host == sponsored

    fun serialize() {
        DataStore.editingId = id
        DataStore.privateStore.putString(Key.name, name)
        DataStore.privateStore.putString(Key.group, url_group)
        DataStore.privateStore.putString(Key.host, host)
        DataStore.privateStore.putString(Key.remotePort, remotePort.toString())
        DataStore.privateStore.putString(Key.password, password)
        DataStore.privateStore.putString(Key.route, route)
        DataStore.privateStore.putString(Key.remoteDns, remoteDns)
        DataStore.privateStore.putString(Key.method, method)
        DataStore.proxyApps = proxyApps
        DataStore.bypass = bypass
        DataStore.privateStore.putBoolean(Key.udpdns, udpdns)
        DataStore.privateStore.putBoolean(Key.ipv6, ipv6)
        DataStore.privateStore.putBoolean(Key.metered, metered)
        DataStore.individual = individual
        DataStore.plugin = plugin ?: ""
        DataStore.udpFallback = udpFallback
        DataStore.privateStore.remove(Key.dirty)
        //add for vmess begin
        DataStore.privateStore.putString(Key.profileType,profileType)
        DataStore.privateStore.putString(Key.alterId,alterId.toString())
        DataStore.privateStore.putString(Key.network,network)
        DataStore.privateStore.putString(Key.headerType,headerType)
        DataStore.privateStore.putString(Key.requestHost,requestHost)
        DataStore.privateStore.putString(Key.path,path)
        DataStore.privateStore.putString(Key.streamSecurity,streamSecurity)
        DataStore.privateStore.putString(Key.allowInsecure,allowInsecure)
        DataStore.privateStore.putString(Key.SNI,SNI)
        DataStore.privateStore.putString(Key.xtlsflow,xtlsflow)
        //add for vmess end
    }

    fun deserialize() {
        check(id == 0L || DataStore.editingId == id)
        DataStore.editingId = null
        // It's assumed that default values are never used, so 0/false/null is always used even if that isn't the case
        name = DataStore.privateStore.getString(Key.name) ?: ""
        url_group = DataStore.privateStore.getString(Key.group) ?: ""
        // It's safe to trim the hostname, as we expect no leading or trailing whitespaces here
        host = (DataStore.privateStore.getString(Key.host) ?: "").trim()
        remotePort = parsePort(DataStore.privateStore.getString(Key.remotePort), 8388, 1)
        password = DataStore.privateStore.getString(Key.password) ?: ""
        method = DataStore.privateStore.getString(Key.method) ?: ""
        route = DataStore.privateStore.getString(Key.route) ?: ""
        remoteDns = DataStore.privateStore.getString(Key.remoteDns) ?: ""
        proxyApps = DataStore.proxyApps
        bypass = DataStore.bypass
        udpdns = DataStore.privateStore.getBoolean(Key.udpdns, false)
        ipv6 = DataStore.privateStore.getBoolean(Key.ipv6, false)
        metered = DataStore.privateStore.getBoolean(Key.metered, false)
        individual = DataStore.individual
        plugin = DataStore.plugin
        udpFallback = DataStore.udpFallback
        //add for v2ray begin
        profileType=DataStore.privateStore.getString(Key.profileType) ?: AppConfig.EConfigType.vless
        alterId=(DataStore.privateStore.getString(Key.alterId)?: "64").toInt()
        network=DataStore.privateStore.getString(Key.network) ?: "tcp"
        headerType=DataStore.privateStore.getString(Key.headerType) ?: ""
        requestHost=DataStore.privateStore.getString(Key.requestHost) ?: ""
        path=DataStore.privateStore.getString(Key.path) ?: ""
        streamSecurity=DataStore.privateStore.getString(Key.streamSecurity) ?: ""
        allowInsecure=DataStore.privateStore.getString(Key.allowInsecure) ?: "false"
        SNI=DataStore.privateStore.getString(Key.SNI) ?: ""
        xtlsflow=DataStore.privateStore.getString(Key.xtlsflow) ?: ""
        //add for v2ray end
    }
    fun isBuiltin(): Boolean {
        return VpnEncrypt.vpnGroupName == url_group
    }
    fun isBuiltin2(): Boolean {
        return VpnEncrypt.freesubGroupName == url_group
    }
}
