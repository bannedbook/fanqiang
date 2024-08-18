package io.nekohasekai.sagernet.utils

import android.content.Context
import io.nekohasekai.sagernet.ktx.app
import libcore.Libcore
import java.io.File

object GeoipUtils {
    /**
     * Generate a rule set for a specific country
     * @param context the context to use
     * @param country the country code to generate the rule set for
     * @return the path to the generated rule set
     */
    fun generateRuleSet(context: Context = app.applicationContext, country: String): String {

        val filesDir = context.getExternalFilesDir(null) ?: context.filesDir

        val ruleSetDir = filesDir.resolve("ruleSets")
        ruleSetDir.mkdirs()
        val output = ruleSetDir.resolve("geoip-$country.srs")

        if (output.isFile) {
            return output.absolutePath
        }

        val geositeFile = File(filesDir, "geoip.db")

        val geoip = Libcore.newGeoip()
        if (!geoip.openGeosite(geositeFile.absolutePath)) {
            error("open geoip failed")
        }

        geoip.convertGeoip(country, output.absolutePath)

        return output.absolutePath
    }
}