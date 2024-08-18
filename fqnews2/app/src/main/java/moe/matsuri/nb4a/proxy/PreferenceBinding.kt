package moe.matsuri.nb4a.proxy

import androidx.preference.Preference
import androidx.preference.PreferenceFragmentCompat
import io.nekohasekai.sagernet.database.DataStore
import io.nekohasekai.sagernet.ktx.Logs
import io.nekohasekai.sagernet.ktx.readableMessage

object Type {
    const val Text = 0
    const val TextToInt = 1
    const val Int = 2
    const val Bool = 3
}

class PreferenceBinding(
    val type: Int = Type.Text,
    var fieldName: String,
    var bean: Any? = null,
    var pf: PreferenceFragmentCompat? = null
) {

    var cacheName = fieldName
    var disable = false

    fun readStringFromCache(): String {
        return DataStore.profileCacheStore.getString(cacheName) ?: ""
    }

    fun readBoolFromCache(): Boolean {
        return DataStore.profileCacheStore.getBoolean(cacheName, false)
    }

    fun readIntFromCache(): Int {
        return DataStore.profileCacheStore.getInt(cacheName, 0)
    }

    fun readStringToIntFromCache(): Int {
        val value = DataStore.profileCacheStore.getString(cacheName)?.toIntOrNull() ?: 0
//        Logs.d("readStringToIntFromCache $value $cacheName -> $fieldName")
        return value
    }

    fun fromCache() {
        if (disable) return
        val f = try {
            bean!!.javaClass.getField(fieldName)
        } catch (e: Exception) {
            Logs.d("binding no field: ${e.readableMessage}")
            return
        }
        when (type) {
            Type.Text -> f.set(bean, readStringFromCache())
            Type.TextToInt -> f.set(bean, readStringToIntFromCache())
            Type.Int -> f.set(bean, readIntFromCache())
            Type.Bool -> f.set(bean, readBoolFromCache())
        }
    }

    fun writeToCache() {
        if (disable) return
        val f = try {
            bean!!.javaClass.getField(fieldName) ?: return
        } catch (e: Exception) {
            Logs.d("binding no field: ${e.readableMessage}")
            return
        }
        val value = f.get(bean)
        when (type) {
            Type.Text -> {
                if (value is String) {
//                    Logs.d("writeToCache TEXT $value $cacheName -> $fieldName")
                    DataStore.profileCacheStore.putString(cacheName, value)
                }
            }
            Type.TextToInt -> {
                if (value is Int) {
//                    Logs.d("writeToCache TEXT2INT $value $cacheName -> $fieldName")
                    DataStore.profileCacheStore.putString(cacheName, value.toString())
                }
            }
            Type.Int -> {
                if (value is Int) {
                    DataStore.profileCacheStore.putInt(cacheName, value)
                }
            }
            Type.Bool -> {
                if (value is Boolean) {
                    DataStore.profileCacheStore.putBoolean(cacheName, value)
                }
            }
        }
    }

    val preference by lazy {
        pf!!.findPreference<Preference>(cacheName)!!
    }
}
