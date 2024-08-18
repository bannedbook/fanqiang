package com.nononsenseapps.feeder.model.opml

import com.nononsenseapps.feeder.archmodel.PREF_VAL_OPEN_WITH_CUSTOM_TAB
import com.nononsenseapps.feeder.archmodel.UserSettings
import com.nononsenseapps.feeder.db.room.Feed
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertEquals
import org.junit.Test
import java.io.ByteArrayOutputStream
import java.net.URL

class OpmlWriterKtTest {
    @Test
    fun escapeAndUnescape() {
        val original = "A \"feeditem\" with id '9' > 0 & < 10"

        val escaped = escape(original)
        val unescaped = unescape(escaped)

        assertEquals(original, unescaped)
        assertEquals(escaped, escape(unescaped))
        assertEquals(original, unescape(escaped))
    }

    @Test
    fun shouldEscapeStrings() =
        runBlocking {
            val bos = ByteArrayOutputStream()
            writeOutputStream(bos, emptyMap(), emptyList(), listOf("quoted \"tag\"")) { tag ->
                val result = mutableListOf<Feed>()
                val feed =
                    Feed(
                        id = 1L,
                        title = "A \"feeditem\" with id '9' > 0 & < 10",
                        customTitle = "A custom \"title\" with id '9' > 0 & < 1e",
                        url = URL("http://example.com/rss.xml?format=feed&type=rss"),
                        tag = tag,
                        notify = true,
                        imageUrl = URL("https://example.com/feedImage"),
                        fullTextByDefault = true,
                        openArticlesWith = "reader",
                        alternateId = true,
                    )

                result.add(feed)
                result
            }
            val output = String(bos.toByteArray())
            assertEquals(expected, output.trimEnd())
        }

    @Test
    fun exportsSettings() =
        runBlocking {
            val bos = ByteArrayOutputStream()
            writeOutputStream(
                os = bos,
                settings = ALL_SETTINGS_WITH_VALUES,
                blockedPatterns = listOf("foo", "break \"xml id '9' > 0 & < 10"),
                tags = listOf("news"),
            ) { tag ->
                val result = mutableListOf<Feed>()
                val feed =
                    Feed(
                        id = 1L,
                        title = "title",
                        customTitle = "customTitle",
                        url = URL("http://example.com/rss.xml?format=feed&type=rss"),
                        tag = tag,
                        notify = true,
                        imageUrl = URL("https://example.com/feedImage"),
                        fullTextByDefault = true,
                        openArticlesWith = "reader",
                        alternateId = true,
                    )

                result.add(feed)
                result
            }
            val output = String(bos.toByteArray())
            assertEquals(expectedWithSettings, output.trimEnd())
        }

    private val expected =
        """
        <?xml version="1.0" encoding="UTF-8"?>
        <opml version="1.1" xmlns:feeder="https://nononsenseapps.com/feeder">
          <head>
            <title>
              Feeder
            </title>
          </head>
          <body>
            <outline title="quoted &quot;tag&quot;" text="quoted &quot;tag&quot;">
              <outline feeder:notify="true" feeder:imageUrl="https://example.com/feedImage" feeder:fullTextByDefault="true" feeder:openArticlesWith="reader" feeder:alternateId="true" title="A custom &quot;title&quot; with id &apos;9&apos; &gt; 0 &amp; &lt; 1e" text="A custom &quot;title&quot; with id &apos;9&apos; &gt; 0 &amp; &lt; 1e" type="rss" xmlUrl="http://example.com/rss.xml?format=feed&amp;type=rss"/>
            </outline>
          </body>
        </opml>
        """.trimIndent()

    private val expectedWithSettings =
        """
        <?xml version="1.0" encoding="UTF-8"?>
        <opml version="1.1" xmlns:feeder="https://nononsenseapps.com/feeder">
          <head>
            <title>
              Feeder
            </title>
          </head>
          <body>
            <outline title="news" text="news">
              <outline feeder:notify="true" feeder:imageUrl="https://example.com/feedImage" feeder:fullTextByDefault="true" feeder:openArticlesWith="reader" feeder:alternateId="true" title="customTitle" text="customTitle" type="rss" xmlUrl="http://example.com/rss.xml?format=feed&amp;type=rss"/>
            </outline>
            <feeder:settings>
              <feeder:setting key="pref_added_feeder_news" value="true"/>
              <feeder:setting key="pref_theme" value="night"/>
              <feeder:setting key="pref_dark_theme" value="DaRk"/>
              <feeder:setting key="pref_dynamic_theme" value="false"/>
              <feeder:setting key="pref_sort" value="oldest_first"/>
              <feeder:setting key="pref_show_fab" value="false"/>
              <feeder:setting key="pref_feed_item_style" value="super_compact"/>
              <feeder:setting key="pref_swipe_as_read" value="DISABLED"/>
              <feeder:setting key="pref_sync_only_charging" value="true"/>
              <feeder:setting key="pref_sync_only_wifi" value="false"/>
              <feeder:setting key="pref_sync_freq" value="720"/>
              <feeder:setting key="pref_sync_on_resume" value="true"/>
              <feeder:setting key="pref_img_only_wifi" value="true"/>
              <feeder:setting key="pref_img_show_thumbnails" value="false"/>
              <feeder:setting key="pref_default_open_item_with" value="3"/>
              <feeder:setting key="pref_open_links_with" value="3"/>
              <feeder:setting key="pref_open_adjacent" value="true"/>
              <feeder:setting key="pref_body_text_scale" value="1.6"/>
              <feeder:setting key="pref_is_mark_as_read_on_scroll" value="true"/>
              <feeder:setting key="pref_readaloud_detect_lang" value="true"/>
              <feeder:setting key="pref_max_lines" value="6"/>
              <feeder:setting key="prefs_filter_saved" value="true"/>
              <feeder:setting key="prefs_filter_recently_read" value="true"/>
              <feeder:setting key="prefs_filter_read" value="false"/>
              <feeder:setting key="prefs_list_show_only_titles" value="true"/>
              <feeder:blocked pattern="foo"/>
              <feeder:blocked pattern="break &quot;xml id &apos;9&apos; &gt; 0 &amp; &lt; 10"/>
            </feeder:settings>
          </body>
        </opml>
        """.trimIndent()

    companion object {
        private val ALL_SETTINGS_WITH_VALUES: Map<String, String> =
            UserSettings.values().associate { userSetting ->
                userSetting.key to
                    when (userSetting) {
                        UserSettings.SETTING_OPEN_LINKS_WITH -> PREF_VAL_OPEN_WITH_CUSTOM_TAB
                        UserSettings.SETTING_ADDED_FEEDER_NEWS -> "true"
                        UserSettings.SETTING_THEME -> "night"
                        UserSettings.SETTING_DARK_THEME -> "DaRk"
                        UserSettings.SETTING_DYNAMIC_THEME -> "false"
                        UserSettings.SETTING_SORT -> "oldest_first"
                        UserSettings.SETTING_SHOW_FAB -> "false"
                        UserSettings.SETTING_FEED_ITEM_STYLE -> "super_compact"
                        UserSettings.SETTING_SWIPE_AS_READ -> "DISABLED"
                        UserSettings.SETTING_SYNC_ON_RESUME -> "true"
                        UserSettings.SETTING_SYNC_ONLY_WIFI -> "false"
                        UserSettings.SETTING_IMG_ONLY_WIFI -> "true"
                        UserSettings.SETTING_IMG_SHOW_THUMBNAILS -> "false"
                        UserSettings.SETTING_DEFAULT_OPEN_ITEM_WITH -> PREF_VAL_OPEN_WITH_CUSTOM_TAB
                        UserSettings.SETTING_TEXT_SCALE -> "1.6"
                        UserSettings.SETTING_IS_MARK_AS_READ_ON_SCROLL -> "true"
                        UserSettings.SETTING_READALOUD_USE_DETECT_LANGUAGE -> "true"
                        UserSettings.SETTING_SYNC_ONLY_CHARGING -> "true"
                        UserSettings.SETTING_SYNC_FREQ -> "720"
                        UserSettings.SETTING_MAX_LINES -> "6"
                        UserSettings.SETTINGS_FILTER_SAVED -> "true"
                        UserSettings.SETTINGS_FILTER_RECENTLY_READ -> "true"
                        UserSettings.SETTINGS_FILTER_READ -> "false"
                        UserSettings.SETTINGS_LIST_SHOW_ONLY_TITLES -> "true"
                        UserSettings.SETTING_OPEN_ADJACENT -> "true"
                    }
            }
    }
}
