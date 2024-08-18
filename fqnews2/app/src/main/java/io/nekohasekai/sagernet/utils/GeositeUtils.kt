package io.nekohasekai.sagernet.utils

import android.content.Context
import io.nekohasekai.sagernet.ktx.app
import libcore.Geosite
import java.io.File

object GeositeUtils {
    /**
     * Generate a rule set for a specific geosite code
     * @param context the context to use
     * @param code the geosite code to generate the rule set for
     * @return the path to the generated rule set
     */
    fun generateRuleSet(context: Context = app.applicationContext, code: String): String {

        val filesDir = context.getExternalFilesDir(null) ?: context.filesDir
        val geositeFile = File(filesDir, "geosite.db")
        val ruleSetDir = filesDir.resolve("ruleSets")
        ruleSetDir.mkdirs()

        val output = ruleSetDir.resolve("geosite-$code.srs")

        val geosite = Geosite()
        if (!geosite.checkGeositeCode(geositeFile.absolutePath, code)) {
            error("code $code not found in geosite")
        }

        geosite.convertGeosite(code, output.absolutePath)

        return output.absolutePath
    }
}