package moe.matsuri.nb4a.proxy

import androidx.preference.PreferenceFragmentCompat


class PreferenceBindingManager {
    val items = mutableListOf<PreferenceBinding>()

    fun add(b: PreferenceBinding): PreferenceBinding {
        items.add(b)
        return b
    }

    fun fromCacheAll(bean: Any) {
        items.forEach {
            it.bean = bean
            it.fromCache()
        }
    }

    fun writeToCacheAll(bean: Any) {
        items.forEach {
            it.bean = bean
            it.writeToCache()
        }
    }

    fun setPreferenceFragment(pf: PreferenceFragmentCompat) {
        items.forEach {
            it.pf = pf
        }
    }

}
