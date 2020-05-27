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

package com.github.shadowsocks.preference
import SpeedUpVPN.VpnEncrypt
import android.os.Binder
import android.util.Log
import androidx.preference.PreferenceDataStore
import com.github.shadowsocks.BootReceiver
import com.github.shadowsocks.Core
import com.github.shadowsocks.database.PrivateDatabase
import com.github.shadowsocks.database.ProfileManager
import com.github.shadowsocks.database.PublicDatabase
import com.github.shadowsocks.net.TcpFastOpen
import com.github.shadowsocks.utils.DirectBoot
import com.github.shadowsocks.utils.Key
import com.github.shadowsocks.utils.parsePort
import java.net.InetSocketAddress
import java.util.*

object DataStore : OnPreferenceDataStoreChangeListener {
    val publicStore = RoomPreferenceDataStore(PublicDatabase.kvPairDao)
    // privateStore will only be used as temp storage for ProfileConfigFragment
    val privateStore = RoomPreferenceDataStore(PrivateDatabase.kvPairDao)

    init {
        publicStore.registerChangeListener(this)
    }

    override fun onPreferenceDataStoreChanged(store: PreferenceDataStore, key: String) {
        when (key) {
            Key.id -> if (directBootAware) DirectBoot.update()
        }
    }

    // hopefully hashCode = mHandle doesn't change, currently this is true from KitKat to Nougat
    private val userIndex by lazy { Binder.getCallingUserHandle().hashCode() }
    private fun getLocalPort(key: String, default: Int): Int {
        val value = publicStore.getInt(key)
        return if (value != null) {
            publicStore.putString(key, value.toString())
            value
        } else parsePort(publicStore.getString(key), default + userIndex)
    }

    var profileId: Long
        get() = publicStore.getLong(Key.id) ?: 0
        set(value) = publicStore.putLong(Key.id, value)
    val persistAcrossReboot get() = publicStore.getBoolean(Key.persistAcrossReboot)
            ?: BootReceiver.enabled.also { publicStore.putBoolean(Key.persistAcrossReboot, it) }
    val canToggleLocked: Boolean get() = publicStore.getBoolean(Key.directBootAware) == true
    val directBootAware: Boolean get() = Core.directBootSupported && canToggleLocked
    val tcpFastOpen: Boolean get() = TcpFastOpen.sendEnabled && publicStore.getBoolean(Key.tfo, false)
    val isAutoUpdateServers: Boolean get() = publicStore.getBoolean(Key.isAutoUpdateServers, true)
    val is_get_free_servers: Boolean get() = publicStore.getBoolean(Key.is_get_free_servers, false)
    val serviceMode get() =  Key.modeProxy //publicStore.getString(Key.serviceMode) ?: Key.modeVpn
    val listenAddress get() = if (publicStore.getBoolean(Key.shareOverLan, false)) "0.0.0.0" else "127.0.0.1"
    var portProxy: Int = VpnEncrypt.SOCK_PROXY_PORT
        //get() = getLocalPort(Key.portProxy, VpnEncrypt.SOCK_PROXY_PORT)
        //set(value) = publicStore.putString(Key.portProxy, value.toString())
    val proxyAddress get() = InetSocketAddress("127.0.0.1", portProxy)
    var portLocalDns: Int
        get() = getLocalPort(Key.portLocalDns, 5450)
        set(value) = publicStore.putString(Key.portLocalDns, value.toString())
    var portTransproxy: Int
        get() = getLocalPort(Key.portTransproxy, 8200)
        set(value) = publicStore.putString(Key.portTransproxy, value.toString())
    var portHttpProxy: Int
        get() = getLocalPort(Key.portHttpProxy, VpnEncrypt.HTTP_PROXY_PORT)
        set(value) = publicStore.putString(Key.portHttpProxy, value.toString())

    /**
     * Initialize settings that have complicated default values.
     */
    fun initGlobal() {
        persistAcrossReboot
        if (publicStore.getBoolean(Key.tfo) == null) publicStore.putBoolean(Key.tfo, tcpFastOpen)
        if (publicStore.getBoolean(Key.isAutoUpdateServers) == null) publicStore.putBoolean(Key.isAutoUpdateServers, isAutoUpdateServers)
        if (publicStore.getBoolean(Key.is_get_free_servers) == null) publicStore.putBoolean(Key.is_get_free_servers, is_get_free_servers)
        //if (publicStore.getString(Key.portProxy) == null) portProxy = portProxy
        if (publicStore.getString(Key.portLocalDns) == null) portLocalDns = portLocalDns
        if (publicStore.getString(Key.portTransproxy) == null) portTransproxy = portTransproxy
        if (publicStore.getString(Key.portHttpProxy) == null) portHttpProxy = portHttpProxy
    }

    var editingId: Long?
        get() = privateStore.getLong(Key.id)
        set(value) = privateStore.putLong(Key.id, value)
    var proxyApps: Boolean
        get() = privateStore.getBoolean(Key.proxyApps) ?: false
        set(value) = privateStore.putBoolean(Key.proxyApps, value)
    var bypass: Boolean
        get() = privateStore.getBoolean(Key.bypass) ?: false
        set(value) = privateStore.putBoolean(Key.bypass, value)
    var individual: String
        get() = privateStore.getString(Key.individual) ?: ""
        set(value) = privateStore.putString(Key.individual, value)
    var plugin: String
        get() = privateStore.getString(Key.plugin) ?: ""
        set(value) = privateStore.putString(Key.plugin, value)
    var udpFallback: Long?
        get() = privateStore.getLong(Key.udpFallback)
        set(value) = privateStore.putLong(Key.udpFallback, value)
    var dirty: Boolean
        get() = privateStore.getBoolean(Key.dirty) ?: false
        set(value) = privateStore.putBoolean(Key.dirty, value)
}
