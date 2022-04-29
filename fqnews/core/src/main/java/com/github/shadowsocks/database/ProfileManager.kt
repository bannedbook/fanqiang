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
import android.database.sqlite.SQLiteCantOpenDatabaseException
import android.util.Base64
import android.util.Log
import android.util.LongSparseArray
import com.github.shadowsocks.Core
import com.github.shadowsocks.Core.defaultDPreference
import com.github.shadowsocks.preference.DataStore
import com.github.shadowsocks.utils.DirectBoot
import com.github.shadowsocks.utils.V2rayConfigUtil
import com.github.shadowsocks.utils.forEachTry
import com.github.shadowsocks.utils.printLog
import com.google.gson.JsonStreamParser
import org.json.JSONArray
import java.io.BufferedReader
import java.io.IOException
import java.io.InputStream
import java.sql.SQLException

/**
 * SQLExceptions are not caught (and therefore will cause crash) for insert/update transactions
 * to ensure we are in a consistent state.
 */
object ProfileManager {
    interface Listener {
        fun onAdd(profile: Profile)
        fun onRemove(profileId: Long)
        fun onCleared()
        fun reloadProfiles()
    }
    var listener: Listener? = null

    @Throws(SQLException::class)
    fun createProfile(profile: Profile = Profile()): Profile {
        var existOne=PrivateDatabase.profileDao.getSameProfile(profile.host,profile.remotePort,profile.path,profile.profileType,profile.streamSecurity,profile.SNI)
        if (existOne==null) {
            profile.id = 0
            profile.userOrder = PrivateDatabase.profileDao.nextOrder() ?: 0
            profile.id = PrivateDatabase.profileDao.create(profile)
            listener?.onAdd(profile)
        }
        else {
            existOne.copyFeatureSettingsTo(profile)
            profile.id=existOne.id
            profile.tx=existOne.tx
            profile.rx=existOne.rx
            profile.elapsed=existOne.elapsed
            updateProfile(profile)
        }
        return profile
    }

    fun deletProfiles(profiles: List<Profile>) {
        val first: Long = Core.currentProfile?.first?.id ?: -1
        val second: Long = Core.currentProfile?.second?.id ?: -1
        profiles.forEach {
            if (it.id != first && it.id != second) delProfile(it.id)
        }
    }

    fun createProfilesFromSub(profiles: List<Profile>, group: String) {
        val old = getAllProfilesByGroup(group).toMutableList()
        profiles.filter {
            for (i: Profile in old) if (it.isSameAs(i)) {
                old.remove(i)
                return@filter false
            }
            return@filter true
        }.forEach { createProfile(it) }
        deletProfiles(old)
    }

    fun createProfilesFromJson(jsons: Sequence<InputStream>, replace: Boolean = false) {
        val profiles = if (replace) getAllProfiles()?.associateBy { it.formattedAddress } else null
        val feature = if (replace) {
            profiles?.values?.singleOrNull { it.id == DataStore.profileId }
        } else Core.currentProfile?.first
        val lazyClear = lazy { clear() }
        jsons.asIterable().forEachTry { json ->
            Profile.parseJson(JsonStreamParser(json.bufferedReader()).asSequence().single(), feature) {
                if (replace) {
                    lazyClear.value
                    // if two profiles has the same address, treat them as the same profile and copy stats over
                    profiles?.get(it.formattedAddress)?.apply {
                        it.tx = tx
                        it.rx = rx
                    }
                }
                createProfile(it)
            }
        }
    }

    fun serializeToJson(profiles: List<Profile>? = getActiveProfiles()): JSONArray? {
        if (profiles == null) return null
        val lookup = LongSparseArray<Profile>(profiles.size).apply { profiles.forEach { put(it.id, it) } }
        return JSONArray(profiles.map { it.toJson(lookup) }.toTypedArray())
    }
    fun serializeToJsonIgnoreVPN(profiles: List<Profile>? = getAllProfilesIgnoreGroup(VpnEncrypt.vpnGroupName)): JSONArray? {
        if (profiles == null) return null
        val lookup = LongSparseArray<Profile>(profiles.size).apply { profiles.forEach { put(it.id, it) } }
        return JSONArray(profiles.map { it.toJson(lookup) }.toTypedArray())
    }

    /**
     * Note: It's caller's responsibility to update DirectBoot profile if necessary.
     */
    @Throws(SQLException::class)
    fun updateProfile(profile: Profile) = try {check(PrivateDatabase.profileDao.update(profile) == 1)}catch (t:Throwable){printLog(t)}

    @Throws(IOException::class)
    fun getProfile(id: Long): Profile? = try {
        PrivateDatabase.profileDao[id]
    } catch (ex: SQLiteCantOpenDatabaseException) {
        throw IOException(ex)
    } catch (ex: SQLException) {
        printLog(ex)
        null
    }

    @Throws(IOException::class)
    fun expand(profile: Profile): Pair<Profile, Profile?> = Pair(profile, profile.udpFallback?.let { getProfile(it) })

    @Throws(SQLException::class)
    fun delProfile(id: Long) {
        try {
            check(PrivateDatabase.profileDao.delete(id) == 1)
            listener?.onRemove(id)
            if (id in Core.activeProfileIds && DataStore.directBootAware) DirectBoot.clean()
        }
        catch (e:Exception){
            Log.e("delProfile",e.toString())
        }
    }

    @Throws(SQLException::class)
    fun clear() = PrivateDatabase.profileDao.deleteAll().also {
        // listener is not called since this won't be used in mobile submodule
        DirectBoot.clean()
        listener?.onCleared()
    }

    @Throws(IOException::class)
    fun ensureNotEmpty() {
        val nonEmpty = try {
            PrivateDatabase.profileDao.isNotEmpty()
        } catch (ex: SQLiteCantOpenDatabaseException) {
            throw IOException(ex)
        } catch (ex: SQLException) {
            printLog(ex)
            false
        }
        if (!nonEmpty) DataStore.profileId = createProfile().id
    }

    @Throws(IOException::class)
    fun getActiveProfiles(): List<Profile>? = try {
        PrivateDatabase.profileDao.listActive()
    } catch (ex: SQLiteCantOpenDatabaseException) {
        throw IOException(ex)
    } catch (ex: SQLException) {
        printLog(ex)
        null
    }

    @Throws(IOException::class)
    fun getProfilesOrderlySpeed(): List<Profile>? = try {
        PrivateDatabase.profileDao.listAllbySpeed()
    } catch (ex: SQLiteCantOpenDatabaseException) {
        throw IOException(ex)
    } catch (ex: SQLException) {
        printLog(ex)
        null
    }

    @Throws(IOException::class)
    fun getAllProfiles(): List<Profile>? = try {
        PrivateDatabase.profileDao.listAll()
    } catch (ex: SQLiteCantOpenDatabaseException) {
        throw IOException(ex)
    } catch (ex: SQLException) {
        printLog(ex)
        null
    }
    @Throws(IOException::class)
    fun getAllProfilesIgnoreGroup(group: String): List<Profile>? = try {
        PrivateDatabase.profileDao.listIgnoreGroup(group)
    } catch (ex: SQLiteCantOpenDatabaseException) {
        throw IOException(ex)
    } catch (ex: SQLException) {
        printLog(ex)
        null
    }

    @Throws(IOException::class)
    fun getAllProfilesByGroup(group: String): List<Profile> = try {
        PrivateDatabase.profileDao.listByGroup(group)
    } catch (ex: SQLiteCantOpenDatabaseException) {
        throw IOException(ex)
    } catch (ex: SQLException) {
        printLog(ex)
        emptyList()
    }

    fun getRandomVPNServer(): Profile? {
        try {
            val profiles=getAllProfilesByGroup(VpnEncrypt.vpnGroupName)
            return if (profiles.isNotEmpty())
                profiles.random()
            else
                getAllProfiles()?.random()
        } catch (ex: Exception) {
            Log.e("getRandomVPNServer",this.javaClass.name+":"+ex.javaClass.name)
            return null
        }
    }

    fun profileToVmessBean(profile: Profile): VmessBean {
        var vmess = VmessBean()
        vmess.configType=profile.profileType
        vmess.guid=profile.id.toString()
        vmess.remoteDns=profile.remoteDns
        vmess.address=profile.host
        vmess.alterId=profile.alterId
        vmess.headerType=profile.headerType
        vmess.id=profile.password
        vmess.network=profile.network
        vmess.path=profile.path
        vmess.port=profile.remotePort
        vmess.remarks= profile.name.toString()
        vmess.requestHost=profile.requestHost
        vmess.security=profile.method
        vmess.streamSecurity=profile.streamSecurity
        vmess.allowInsecure=profile.allowInsecure
        vmess.SNI=profile.SNI
        vmess.flow = profile.xtlsflow
        vmess.subid=profile.url_group
        vmess.testResult=profile.elapsed.toString()

        if(profile.route=="all")vmess.route="0"
        else if(profile.route=="bypass-lan")vmess.route="1"
        else if(profile.route=="bypass-china")vmess.route="2"
        else if(profile.route=="bypass-lan-china")vmess.route="3"
        else vmess.route="0"

        return vmess
    }
    /**
     * gen and store v2ray config file
     */
    fun genStoreV2rayConfig(activeProfile:Profile,isTest:Boolean=false): Boolean {
        try {
            val result = V2rayConfigUtil.getV2rayConfig(Core.app, profileToVmessBean(activeProfile),isTest)
            if (result.status) {
                defaultDPreference.setPrefString(AppConfig.PREF_CURR_CONFIG, result.content)
                defaultDPreference.setPrefString(AppConfig.PREF_CURR_CONFIG_GUID, activeProfile.id.toString())
                defaultDPreference.setPrefString(AppConfig.PREF_CURR_CONFIG_NAME, activeProfile.name)
                return true
            } else {
                return false
            }
        } catch (e: Exception) {
            e.printStackTrace()
            return false
        }
    }
}
