package com.nononsenseapps.feeder.model

import org.intellij.lang.annotations.Language
import java.io.InputStream

class Feeds {
    companion object {
        val nixosRss: InputStream
            get() = Companion::class.java.getResourceAsStream("rss_nixos.xml")!!

        val cowboyJson: String
            get() =
                String(
                    Companion::class.java.getResourceAsStream("cowboyprogrammer_feed.json")!!
                        .use { it.readBytes() },
                )

        val cowboyAtom: String
            get() =
                String(
                    Companion::class.java.getResourceAsStream("cowboyprogrammer_atom.xml")!!.use { it.readBytes() },
                )

        /**
         * Reported in https://gitlab.com/spacecowboy/Feeder/-/issues/410
         *
         * All items are genuine - except the first which is taken from the feed at 2021-10-06T21:06:00
         */
        @Language("xml")
        const val RSS_WITH_DUPLICATE_GUIDS = """
<rss xmlns:atom="http://www.w3.org/2005/Atom" version="2.0">
<channel>
<title>Weather Warnings for Victoria. Issued by the Australian Bureau of Meteorology</title>
<link>http://www.bom.gov.au/fwo/IDZ00059.warnings_vic.xml</link>
<atom:link href="http://www.bom.gov.au/fwo/IDZ00059.warnings_vic.xml" rel="self" type="application/rss+xml"/>
<description>Current weather warnings for Victoria, Australia including strong wind, gale, storm force and hurricane force wind warnings; tsunami; damaging waves; abnormally high tides; tropical cyclones; severe thunderstorms; severe weather; fire weather; flood; frost; driving; bushwalking; sheep graziers and other agricultural warnings.</description>
<language>en-au</language>
<copyright>Copyright: (C) Copyright Commonwealth of Australia 2010, Bureau of Meteorology (ABN 92637 533532), see http://www.bom.gov.au/other/copyright.shtml for terms and conditions of reuse.</copyright>
<webMaster>webops@bom.gov.au (Help desk)</webMaster>
<pubDate>Tue, 05 Oct 2021 11:02:55 GMT</pubDate>
<lastBuildDate>Tue, 05 Oct 2021 11:02:55 GMT</lastBuildDate>
<generator>Australian Bureau of Meteorology</generator>
<ttl>10</ttl>
<image>
<url>http://www.bom.gov.au/images/bom_logo_inline.gif</url>
<title>Weather Warnings for Victoria. Issued by the Australian Bureau of Meteorology</title>
<link>http://www.bom.gov.au/fwo/IDZ00059.warnings_vic.xml</link>
</image>
<item>
<title>08/10:22 EDT Minor Flood Warning for the Murray River</title>
<link>http://www.bom.gov.au/nsw/warnings/flood/murrayriver.shtml</link>
<pubDate>Thu, 07 Oct 2021 23:22:07 GMT</pubDate>
<guid isPermaLink="false">http://www.bom.gov.au/nsw/warnings/flood/murrayriver.shtml</guid>
</item>
<item>
<title>05/10:44 EDT Minor Flood Warning for the Murray River</title>
<link>http://www.bom.gov.au/nsw/warnings/flood/murrayriver.shtml</link>
<pubDate>Mon, 04 Oct 2021 23:44:01 GMT</pubDate>
<guid isPermaLink="false">http://www.bom.gov.au/nsw/warnings/flood/murrayriver.shtml</guid>
</item>
<item>
<title>05/22:00 EDT Marine Wind Warning Summary for Victoria</title>
<link>http://www.bom.gov.au/vic/warnings/marinewind.shtml</link>
<pubDate>Tue, 05 Oct 2021 11:00:19 GMT</pubDate>
<guid isPermaLink="false">http://www.bom.gov.au/vic/warnings/marinewind.shtml</guid>
</item>
<item>
<title>05/16:04 EDT Cancellation of Severe Weather Warning for East Gippsland, North East and West and South Gippsland Forecast Districts. </title>
<link>http://www.bom.gov.au/products/IDV21037.shtml</link>
<pubDate>Tue, 05 Oct 2021 05:04:04 GMT</pubDate>
<guid isPermaLink="false">http://www.bom.gov.au/products/IDV21037.shtml</guid>
</item>
<item>
<title>05/10:32 EDT Frost Warning for North Central, North East and Central forecast districts</title>
<link>http://www.bom.gov.au/vic/warnings/frost.shtml</link>
<pubDate>Mon, 04 Oct 2021 23:32:38 GMT</pubDate>
<guid isPermaLink="false">http://www.bom.gov.au/vic/warnings/frost.shtml</guid>
</item>
<item>
<title>05/15:32 EDT Cancellation of Warning to Sheep Graziers for Wimmera, North Central, North East, South West, Central and West and South Gippsland forecast districts</title>
<link>http://www.bom.gov.au/vic/warnings/sheep.shtml</link>
<pubDate>Tue, 05 Oct 2021 04:32:44 GMT</pubDate>
<guid isPermaLink="false">http://www.bom.gov.au/vic/warnings/sheep.shtml</guid>
</item>
<item>
<title>05/13:48 EDT Final Flood Watch for Barwon, Otway Coast, Greater Melbourne, South and West Gippsland and North East Victoria</title>
<link>http://www.bom.gov.au/cgi-bin/wrap_fwo.pl?IDV35010.html</link>
<pubDate>Tue, 05 Oct 2021 02:48:06 GMT</pubDate>
<guid isPermaLink="false">http://www.bom.gov.au/cgi-bin/wrap_fwo.pl?IDV35010.html</guid>
</item>
<item>
<title>05/17:11 EDT Flood Warning Summary for Victoria</title>
<link>http://www.bom.gov.au/vic/warnings/flood/summary.shtml</link>
<pubDate>Tue, 05 Oct 2021 06:11:32 GMT</pubDate>
<guid isPermaLink="false">http://www.bom.gov.au/vic/warnings/flood/summary.shtml</guid>
</item>
<item>
<title>05/11:16 EDT Minor Flood Warning for the Thomson River</title>
<link>http://www.bom.gov.au/vic/warnings/flood/thomsonriver.shtml</link>
<pubDate>Tue, 05 Oct 2021 00:16:26 GMT</pubDate>
<guid isPermaLink="false">http://www.bom.gov.au/vic/warnings/flood/thomsonriver.shtml</guid>
</item>
<item>
<title>05/10:13 EDT Minor Flood Warning for the Latrobe River</title>
<link>http://www.bom.gov.au/vic/warnings/flood/latroberiver.shtml</link>
<pubDate>Mon, 04 Oct 2021 23:13:47 GMT</pubDate>
<guid isPermaLink="false">http://www.bom.gov.au/vic/warnings/flood/latroberiver.shtml</guid>
</item>
<item>
<title>05/16:06 EDT Minor Flood Warning for the Yarra River</title>
<link>http://www.bom.gov.au/vic/warnings/flood/yarrariver.shtml</link>
<pubDate>Tue, 05 Oct 2021 05:06:20 GMT</pubDate>
<guid isPermaLink="false">http://www.bom.gov.au/vic/warnings/flood/yarrariver.shtml</guid>
</item>
<item>
<title>05/17:11 EDT Minor Flood Warning for the Bunyip River</title>
<link>http://www.bom.gov.au/cgi-bin/wrap_fwo.pl?IDV36330.html</link>
<pubDate>Tue, 05 Oct 2021 06:11:14 GMT</pubDate>
<guid isPermaLink="false">http://www.bom.gov.au/cgi-bin/wrap_fwo.pl?IDV36330.html</guid>
</item>
<item>
<title>05/12:37 EDT Minor Flood Warning for the Kiewa River</title>
<link>http://www.bom.gov.au/vic/warnings/flood/kiewariver.shtml</link>
<pubDate>Tue, 05 Oct 2021 01:37:01 GMT</pubDate>
<guid isPermaLink="false">http://www.bom.gov.au/vic/warnings/flood/kiewariver.shtml</guid>
</item>
</channel>
</rss>
"""
    }
}
