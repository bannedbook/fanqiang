package moe.matsuri.nb4a

import io.nekohasekai.sagernet.database.DataStore
import io.nekohasekai.sagernet.utils.GeoipUtils
import io.nekohasekai.sagernet.utils.GeositeUtils
import moe.matsuri.nb4a.SingBoxOptions.RuleSet

object SingBoxOptionsUtil {

    fun domainStrategy(tag: String): String {
        fun auto2AsIs(key: String): String {
            return (DataStore.configurationStore.getString(key) ?: "").replace("auto", "")
        }
        return when (tag) {
            "dns-remote" -> {
                auto2AsIs("domain_strategy_for_remote")
            }

            "dns-direct" -> {
                auto2AsIs("domain_strategy_for_direct")
            }

            // server
            else -> {
                auto2AsIs("domain_strategy_for_server")
            }
        }
    }

}

fun SingBoxOptions.DNSRule_DefaultOptions.makeSingBoxRule(list: List<String>) {
    geosite = mutableListOf<String>()
    domain = mutableListOf<String>()
    domain_suffix = mutableListOf<String>()
    domain_regex = mutableListOf<String>()
    domain_keyword = mutableListOf<String>()
    list.forEach {
        if (it.startsWith("geosite:")) {
            geosite.plusAssign(it.removePrefix("geosite:"))
        } else if (it.startsWith("full:")) {
            domain.plusAssign(it.removePrefix("full:").lowercase())
        } else if (it.startsWith("domain:")) {
            domain_suffix.plusAssign(it.removePrefix("domain:").lowercase())
        } else if (it.startsWith("regexp:")) {
            domain_regex.plusAssign(it.removePrefix("regexp:").lowercase())
        } else if (it.startsWith("keyword:")) {
            domain_keyword.plusAssign(it.removePrefix("keyword:").lowercase())
        } else {
            // https://github.com/SagerNet/sing-box/commit/5d41e328d4a9f7549dd27f11b4ccc43710a73664
            domain.plusAssign(it.lowercase())
        }
    }
    geosite?.removeIf { it.isNullOrBlank() }
    domain?.removeIf { it.isNullOrBlank() }
    domain_suffix?.removeIf { it.isNullOrBlank() }
    domain_regex?.removeIf { it.isNullOrBlank() }
    domain_keyword?.removeIf { it.isNullOrBlank() }
    if (geosite?.isEmpty() == true) geosite = null
    if (domain?.isEmpty() == true) domain = null
    if (domain_suffix?.isEmpty() == true) domain_suffix = null
    if (domain_regex?.isEmpty() == true) domain_regex = null
    if (domain_keyword?.isEmpty() == true) domain_keyword = null
}

fun SingBoxOptions.DNSRule_DefaultOptions.checkEmpty(): Boolean {
    if (geosite?.isNotEmpty() == true) return false
    if (domain?.isNotEmpty() == true) return false
    if (domain_suffix?.isNotEmpty() == true) return false
    if (domain_regex?.isNotEmpty() == true) return false
    if (domain_keyword?.isNotEmpty() == true) return false
    if (user_id?.isNotEmpty() == true) return false
    return true
}

fun SingBoxOptions.Rule_DefaultOptions.generateRuleSet(ruleSet: MutableList<RuleSet>) {
    rule_set?.forEach {
        when {
            it.startsWith("geoip") -> {
                val geoipPath = GeoipUtils.generateRuleSet(country = it.removePrefix("geoip:"))
                ruleSet.add(RuleSet().apply {
                    type = "local"
                    tag = it
                    format = "binary"
                    path = geoipPath
                })
            }

            it.startsWith("geosite") -> {
                val geositePath = GeositeUtils.generateRuleSet(code = it.removePrefix("geosite:"))
                ruleSet.add(RuleSet().apply {
                    type = "local"
                    tag = it
                    format = "binary"
                    path = geositePath
                })
            }
        }
    }
}

fun SingBoxOptions.Rule_DefaultOptions.makeSingBoxRule(list: List<String>, isIP: Boolean) {
    if (isIP) {
        ip_cidr = mutableListOf<String>()
        rule_set = mutableListOf<String>()
    } else {
        rule_set = mutableListOf<String>()
        domain = mutableListOf<String>()
        domain_suffix = mutableListOf<String>()
        domain_regex = mutableListOf<String>()
        domain_keyword = mutableListOf<String>()
    }
    list.forEach {
        if (isIP) {
            if (it.startsWith("geoip:")) {
                rule_set.plusAssign(it)
                rule_set_ipcidr_match_source = true
            } else {
                ip_cidr.plusAssign(it)
            }
            return@forEach
        }
        if (it.startsWith("geosite:")) {
            rule_set.plusAssign(it)
        } else if (it.startsWith("full:")) {
            domain.plusAssign(it.removePrefix("full:").lowercase())
        } else if (it.startsWith("domain:")) {
            domain_suffix.plusAssign(it.removePrefix("domain:").lowercase())
        } else if (it.startsWith("regexp:")) {
            domain_regex.plusAssign(it.removePrefix("regexp:").lowercase())
        } else if (it.startsWith("keyword:")) {
            domain_keyword.plusAssign(it.removePrefix("keyword:").lowercase())
        } else {
            // https://github.com/SagerNet/sing-box/commit/5d41e328d4a9f7549dd27f11b4ccc43710a73664
            domain.plusAssign(it.lowercase())
        }
    }
    ip_cidr?.removeIf { it.isNullOrBlank() }
    rule_set?.removeIf { it.isNullOrBlank() }
    domain?.removeIf { it.isNullOrBlank() }
    domain_suffix?.removeIf { it.isNullOrBlank() }
    domain_regex?.removeIf { it.isNullOrBlank() }
    domain_keyword?.removeIf { it.isNullOrBlank() }
    if (ip_cidr?.isEmpty() == true) ip_cidr = null
    if (domain?.isEmpty() == true) domain = null
    if (domain_suffix?.isEmpty() == true) domain_suffix = null
    if (domain_regex?.isEmpty() == true) domain_regex = null
    if (domain_keyword?.isEmpty() == true) domain_keyword = null
}

fun SingBoxOptions.Rule_DefaultOptions.checkEmpty(): Boolean {
    if (ip_cidr?.isNotEmpty() == true) return false
    if (domain?.isNotEmpty() == true) return false
    if (rule_set?.isNotEmpty() == true) return false
    if (domain_suffix?.isNotEmpty() == true) return false
    if (domain_regex?.isNotEmpty() == true) return false
    if (domain_keyword?.isNotEmpty() == true) return false
    if (user_id?.isNotEmpty() == true) return false
    //
    if (port?.isNotEmpty() == true) return false
    if (port_range?.isNotEmpty() == true) return false
    if (source_ip_cidr?.isNotEmpty() == true) return false
    return true
}
