package com.nononsenseapps.feeder.db.room

import android.content.Context
import android.content.SharedPreferences
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteException
import android.util.Log
import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase
import androidx.room.TypeConverters
import androidx.room.migration.Migration
import androidx.sqlite.db.SupportSQLiteDatabase
import jww.app.FeederApplication
import com.nononsenseapps.feeder.archmodel.Repository
import com.nononsenseapps.feeder.blob.blobOutputStream
import com.nononsenseapps.feeder.crypto.AesCbcWithIntegrity
import com.nononsenseapps.feeder.util.FilePathProvider
import com.nononsenseapps.feeder.util.contentValues
import com.nononsenseapps.feeder.util.forEach
import com.nononsenseapps.feeder.util.setInt
import com.nononsenseapps.feeder.util.setLong
import com.nononsenseapps.feeder.util.setString
import org.kodein.di.DI
import org.kodein.di.DIAware
import org.kodein.di.android.closestDI
import org.kodein.di.instance

const val DATABASE_NAME = "rssDatabase"
const val ID_UNSET: Long = 0
const val ID_ALL_FEEDS: Long = -10
const val ID_SAVED_ARTICLES: Long = -20

private const val LOG_TAG = "FEEDER_APPDB"

/**
 * Database versions
 * 4: Was using the RSS Server
 * 5: Added feed url field to feed_item
 * 6: Added feed icon field to feeds
 * 7: Migration to Room
 */
@Database(
    entities = [
        Feed::class,
        FeedItem::class,
        BlocklistEntry::class,
        SyncRemote::class,
        ReadStatusSynced::class,
        RemoteReadMark::class,
        RemoteFeed::class,
        SyncDevice::class,
    ],
    views = [
        FeedsWithItemsForNavDrawer::class,
    ],
    version = 36,
)
@TypeConverters(Converters::class)
abstract class AppDatabase : RoomDatabase() {
    abstract fun feedDao(): FeedDao

    abstract fun feedItemDao(): FeedItemDao

    abstract fun blocklistDao(): BlocklistDao

    abstract fun syncRemoteDao(): SyncRemoteDao

    abstract fun readStatusSyncedDao(): ReadStatusSyncedDao

    abstract fun remoteReadMarkDao(): RemoteReadMarkDao

    abstract fun remoteFeedDao(): RemoteFeedDao

    abstract fun syncDeviceDao(): SyncDeviceDao

    companion object {
        // For Singleton instantiation
        @Volatile
        private var instance: AppDatabase? = null

        /**
         * Use this in tests only
         */
        internal fun setInstance(db: AppDatabase) {
            instance = db
        }

        fun getInstance(context: Context): AppDatabase {
            return instance ?: synchronized(this) {
                instance ?: buildDatabase(context).also { instance = it }
            }
        }

        private fun buildDatabase(context: Context): AppDatabase {
            val di: DI by closestDI(context)
            return Room.databaseBuilder(context, AppDatabase::class.java, DATABASE_NAME)
                .addMigrations(*getAllMigrations(di))
                .build()
        }
    }
}

// 17-20 were never part of any release, just made for easier testing
fun getAllMigrations(di: DI) =
    arrayOf(
        MIGRATION_5_7,
        MIGRATION_6_7,
        MIGRATION_7_8,
        MIGRATION_8_9,
        MIGRATION_9_10,
        MIGRATION_10_11,
        MIGRATION_11_12,
        MIGRATION_12_13,
        MIGRATION_13_14,
        MIGRATION_14_15,
        MIGRATION_15_16,
        MIGRATION_16_17,
        MIGRATION_17_18,
        MIGRATION_18_19,
        MIGRATION_19_20,
        MIGRATION_20_21,
        MIGRATION_21_22,
        MIGRATION_22_23,
        MigrationFrom23To24(di),
        MigrationFrom24To25(di),
        MigrationFrom25To26(di),
        MigrationFrom26To27(di),
        MigrationFrom27To28(di),
        MigrationFrom28To29(di),
        MigrationFrom29To30(di),
        MigrationFrom30To31(di),
        MigrationFrom31To32(di),
        MigrationFrom32To33(di),
        MigrationFrom33To34(di),
        MigrationFrom34To35(di),
        MigrationFrom35To36(di),
    )

/*
 * 6 represents legacy database
 * 7 represents new Room database
 */
class MigrationFrom35To36(override val di: DI) : Migration(35, 36), DIAware {
    override fun migrate(database: SupportSQLiteDatabase) {
        // TODO add column retry_after to feeds, default epoch, not null
        database.execSQL(
            """
            ALTER TABLE feeds ADD COLUMN retry_after INTEGER NOT NULL DEFAULT 0
            """.trimIndent(),
        )
    }
}

class MigrationFrom34To35(override val di: DI) : Migration(34, 35), DIAware {
    private val repository: Repository by instance()

    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL("drop view feeds_with_items_for_nav_drawer")

        database.execSQL("alter table feed_items add column block_time integer default null")
        database.execSQL("create index index_feed_items_block_time on feed_items (block_time)")

        // Room schema is anal about whitespace
        @Suppress("ktlint:standard:max-line-length")
        val sql = "CREATE VIEW `feeds_with_items_for_nav_drawer` AS select feeds.id as feed_id, item_id, case when custom_title is '' then title else custom_title end as display_title, tag, image_url, unread, bookmarked\n    from feeds\n    left join (\n        select id as item_id, feed_id, read_time is null as unread, bookmarked\n        from feed_items\n        where block_time is null\n    )\n    ON feeds.id = feed_id"
        database.execSQL(sql)

        repository.scheduleBlockListUpdate(0)
    }
}

class MigrationFrom33To34(override val di: DI) : Migration(33, 34), DIAware {
    override fun migrate(database: SupportSQLiteDatabase) {
        // Room schema is anal about whitespace
        @Suppress("ktlint:standard:max-line-length")
        val sql = "CREATE VIEW `feeds_with_items_for_nav_drawer` AS select feeds.id as feed_id, item_id, case when custom_title is '' then title else custom_title end as display_title, tag, image_url, unread, bookmarked\n    from feeds\n    left join (\n        select id as item_id, feed_id, read_time is null as unread, bookmarked\n        from feed_items\n        where not exists(select 1 from blocklist where lower(feed_items.plain_title) glob blocklist.glob_pattern)\n    )\n    ON feeds.id = feed_id"
        database.execSQL(sql)
    }
}

class MigrationFrom32To33(override val di: DI) : Migration(32, 33), DIAware {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            alter table feeds add column skip_duplicates integer not null default 0
            """.trimIndent(),
        )
    }
}

class MigrationFrom31To32(override val di: DI) : Migration(31, 32), DIAware {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            alter table feed_items add column image_from_body integer not null default 0
            """.trimIndent(),
        )
    }
}

class MigrationFrom30To31(override val di: DI) : Migration(30, 31), DIAware {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            alter table feed_items add column word_count_full integer not null default 0
            """.trimIndent(),
        )
    }
}

class MigrationFrom29To30(override val di: DI) : Migration(29, 30), DIAware {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            alter table feed_items add column word_count integer not null default 0
            """.trimIndent(),
        )
    }
}

class MigrationFrom28To29(override val di: DI) : Migration(28, 29), DIAware {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            alter table feed_items add column enclosure_type text
            """.trimIndent(),
        )
    }
}

class MigrationFrom27To28(override val di: DI) : Migration(27, 28), DIAware {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            alter table feeds add column site_fetched integer not null default 0
            """.trimIndent(),
        )
    }
}

class MigrationFrom26To27(override val di: DI) : Migration(26, 27), DIAware {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            ALTER TABLE feed_items ADD COLUMN read_time INTEGER DEFAULT null
            """.trimIndent(),
        )

        database.execSQL(
            """
            update feed_items
                set read_time = 1690317917000
            where unread = 0;
            """.trimIndent(),
        )
    }
}

class MigrationFrom25To26(override val di: DI) : Migration(25, 26), DIAware {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            CREATE UNIQUE INDEX idx_feed_items_cursor
            ON feed_items (primary_sort_time, pub_date, id)
            """.trimIndent(),
        )

        database.execSQL(
            """
            update feed_items
                set bookmarked = 1
            where pinned = 1;
            """.trimIndent(),
        )
    }
}

class MigrationFrom24To25(override val di: DI) : Migration(24, 25), DIAware {
    private val filePathProvider: FilePathProvider by instance()

    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            ALTER TABLE feed_items ADD COLUMN fulltext_downloaded INTEGER NOT NULL DEFAULT 0
            """.trimIndent(),
        )

        // Delete all existing full text
        filePathProvider.filesDir.list { _, name ->
            name.endsWith(".full.html.gz")
        }?.forEach { name ->
            try {
                filePathProvider.filesDir.resolve(name).delete()
            } catch (t: Throwable) {
                Log.e(LOG_TAG, "Failed to delete: $name")
            }
        }

        // Move all article texts to new location
        filePathProvider.filesDir.list { _, name ->
            name.endsWith(".txt.gz")
        }?.forEach { name ->
            try {
                val src = filePathProvider.filesDir.resolve(name)
                val dst = filePathProvider.articleDir.resolve(name)

                if (!filePathProvider.articleDir.isDirectory) {
                    filePathProvider.articleDir.mkdirs()
                }

                src.renameTo(dst)
            } catch (t: Throwable) {
                Log.e(LOG_TAG, "Failed to delete: $name")
            }
        }
    }
}

class MigrationFrom23To24(override val di: DI) : Migration(23, 24), DIAware {
    private val sharedPrefs: SharedPreferences by instance()

    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            CREATE TABLE IF NOT EXISTS `blocklist`
                (`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, 
                 `glob_pattern` TEXT NOT NULL)
            """.trimIndent(),
        )

        database.execSQL(
            """
            CREATE UNIQUE INDEX IF NOT EXISTS `index_blocklist_glob_pattern` on `blocklist` (`glob_pattern`)
            """.trimIndent(),
        )

        val blocks =
            sharedPrefs.getStringSet("pref_block_list_values", null)
                ?: emptySet()

        if (blocks.isNotEmpty()) {
            // ('*foo*'), ('*bar*'), ('*car*')
            val valuesList = blocks.joinToString(separator = ",") { "('*$it*')" }

            try {
                database.execSQL(
                    """
                    INSERT INTO `blocklist`
                        (`glob_pattern`)
                        VALUES $valuesList
                    """.trimIndent(),
                )
            } catch (e: SQLiteException) {
                Log.e("FEEDER_DB", "Failed to migrate blocklist", e)
            }

            sharedPrefs.edit()
                .remove("pref_block_list_values")
                .apply()
        }
    }
}

@Suppress("ClassName")
object MIGRATION_22_23 : Migration(22, 23) {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            ALTER TABLE feed_items ADD COLUMN bookmarked INTEGER NOT NULL DEFAULT 0
            """.trimIndent(),
        )
    }
}

@Suppress("ClassName")
object MIGRATION_21_22 : Migration(21, 22) {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            ALTER TABLE feed_items ADD COLUMN pinned INTEGER NOT NULL DEFAULT 0
            """.trimIndent(),
        )
    }
}

@Suppress("ClassName")
object MIGRATION_20_21 : Migration(20, 21) {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            ALTER TABLE sync_remote
              ADD COLUMN secret_key TEXT NOT NULL DEFAULT ''
            """.trimIndent(),
        )
        database.execSQL(
            """
            ALTER TABLE sync_remote
              ADD COLUMN last_feeds_remote_hash INTEGER NOT NULL DEFAULT 0
            """.trimIndent(),
        )
        database.execSQL(
            """
            ALTER TABLE feeds
              ADD COLUMN when_modified INTEGER NOT NULL DEFAULT 0
            """.trimIndent(),
        )
        database.execSQL(
            """
            CREATE TABLE IF NOT EXISTS `remote_feed` (`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, `sync_remote` INTEGER NOT NULL, `url` TEXT NOT NULL, FOREIGN KEY(`sync_remote`) REFERENCES `sync_remote`(`id`) ON UPDATE NO ACTION ON DELETE CASCADE )
            """.trimIndent(),
        )
        database.execSQL(
            """
            CREATE UNIQUE INDEX IF NOT EXISTS `index_remote_feed_sync_remote_url` ON `remote_feed` (`sync_remote`, `url`)
            """.trimIndent(),
        )
        database.execSQL(
            """
            CREATE INDEX IF NOT EXISTS `index_remote_feed_url` ON `remote_feed` (`url`)
            """.trimIndent(),
        )
        database.execSQL(
            """
            CREATE INDEX IF NOT EXISTS `index_remote_feed_sync_remote` ON `remote_feed` (`sync_remote`)
            """.trimIndent(),
        )
        // And generate encryption key
        database.execSQL(
            """
            UPDATE sync_remote
            SET secret_key = ?
            WHERE id IS 1
            """.trimIndent(),
            arrayOf(AesCbcWithIntegrity.generateKey().toString()),
        )
    }
}

@Suppress("ClassName")
object MIGRATION_19_20 : Migration(19, 20) {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            ALTER TABLE sync_remote 
              ADD COLUMN device_id INTEGER NOT NULL DEFAULT 0
            """.trimIndent(),
        )
        database.execSQL(
            """
            ALTER TABLE sync_remote 
              ADD COLUMN device_name TEXT NOT NULL DEFAULT ''
            """.trimIndent(),
        )
        database.execSQL(
            """
            CREATE TABLE IF NOT EXISTS `sync_device` (`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, `sync_remote` INTEGER NOT NULL, `device_id` INTEGER NOT NULL, `device_name` TEXT NOT NULL, FOREIGN KEY(`sync_remote`) REFERENCES `sync_remote`(`id`) ON UPDATE NO ACTION ON DELETE CASCADE )
            """.trimIndent(),
        )
        database.execSQL(
            """
            CREATE UNIQUE INDEX IF NOT EXISTS `index_sync_device_sync_remote_device_id` ON `sync_device` (`sync_remote`, `device_id`)
            """.trimIndent(),
        )
        database.execSQL(
            """
            CREATE INDEX IF NOT EXISTS `index_sync_device_sync_remote` ON `sync_device` (`sync_remote`)
            """.trimIndent(),
        )
    }
}

@Suppress("ClassName")
object MIGRATION_18_19 : Migration(18, 19) {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            CREATE TABLE IF NOT EXISTS `remote_read_mark` (`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, `sync_remote` INTEGER NOT NULL, `feed_url` TEXT NOT NULL, `guid` TEXT NOT NULL, `timestamp` INTEGER NOT NULL, FOREIGN KEY(`sync_remote`) REFERENCES `sync_remote`(`id`) ON UPDATE NO ACTION ON DELETE CASCADE )
            """.trimIndent(),
        )
        database.execSQL(
            """
            CREATE UNIQUE INDEX IF NOT EXISTS `index_remote_read_mark_sync_remote_feed_url_guid` ON `remote_read_mark` (`sync_remote`, `feed_url`, `guid`)
            """.trimIndent(),
        )
        database.execSQL(
            """
            CREATE INDEX IF NOT EXISTS `index_remote_read_mark_feed_url_guid` ON `remote_read_mark` (`feed_url`, `guid`)
            """.trimIndent(),
        )
        database.execSQL(
            """
            CREATE INDEX IF NOT EXISTS `index_remote_read_mark_sync_remote` ON `remote_read_mark` (`sync_remote`)
            """.trimIndent(),
        )
        database.execSQL(
            """
            CREATE INDEX IF NOT EXISTS `index_remote_read_mark_timestamp` ON `remote_read_mark` (`timestamp`)
            """.trimIndent(),
        )
    }
}

@Suppress("ClassName")
object MIGRATION_17_18 : Migration(17, 18) {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            CREATE TABLE IF NOT EXISTS `read_status_synced` (`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, `sync_remote` INTEGER NOT NULL, `feed_item` INTEGER NOT NULL, FOREIGN KEY(`feed_item`) REFERENCES `feed_items`(`id`) ON UPDATE NO ACTION ON DELETE CASCADE , FOREIGN KEY(`sync_remote`) REFERENCES `sync_remote`(`id`) ON UPDATE NO ACTION ON DELETE CASCADE )
            """.trimIndent(),
        )
        database.execSQL(
            """
            CREATE UNIQUE INDEX IF NOT EXISTS `index_read_status_synced_feed_item_sync_remote` ON `read_status_synced` (`feed_item`, `sync_remote`)
            """.trimIndent(),
        )
        database.execSQL(
            """
            CREATE INDEX IF NOT EXISTS `index_read_status_synced_feed_item` ON `read_status_synced` (`feed_item`);
            """.trimIndent(),
        )
        database.execSQL(
            """
            CREATE INDEX IF NOT EXISTS `index_read_status_synced_sync_remote` ON `read_status_synced` (`sync_remote`);
            """.trimIndent(),
        )
    }
}

@Suppress("ClassName")
object MIGRATION_16_17 : Migration(16, 17) {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            CREATE TABLE sync_remote (`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, `url` TEXT NOT NULL, `sync_chain_id` TEXT NOT NULL, `latest_message_timestamp` INTEGER NOT NULL);
            """.trimIndent(),
        )
    }
}

@Suppress("ClassName")
object MIGRATION_15_16 : Migration(15, 16) {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            ALTER TABLE feeds ADD COLUMN currently_syncing INTEGER NOT NULL DEFAULT 0
            """.trimIndent(),
        )
    }
}

@Suppress("ClassName")
object MIGRATION_14_15 : Migration(14, 15) {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            ALTER TABLE feeds ADD COLUMN alternate_id INTEGER NOT NULL DEFAULT 0
            """.trimIndent(),
        )
    }
}

@Suppress("ClassName")
object MIGRATION_13_14 : Migration(13, 14) {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            ALTER TABLE feeds ADD COLUMN open_articles_with TEXT NOT NULL DEFAULT ''
            """.trimIndent(),
        )
    }
}

@Suppress("ClassName")
object MIGRATION_12_13 : Migration(12, 13) {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            ALTER TABLE feeds ADD COLUMN fulltext_by_default INTEGER NOT NULL DEFAULT 0
            """.trimIndent(),
        )
    }
}

@Suppress("ClassName")
object MIGRATION_11_12 : Migration(11, 12) {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            ALTER TABLE feed_items ADD COLUMN primary_sort_time INTEGER NOT NULL DEFAULT 0
            """.trimIndent(),
        )
    }
}

@Suppress("ClassName")
object MIGRATION_10_11 : Migration(10, 11) {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            ALTER TABLE feed_items ADD COLUMN first_synced_time INTEGER NOT NULL DEFAULT 0
            """.trimIndent(),
        )
    }
}

@Suppress("ClassName")
object MIGRATION_9_10 : Migration(9, 10) {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            CREATE TABLE IF NOT EXISTS `feed_items_new` (`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, `guid` TEXT NOT NULL, `title` TEXT NOT NULL, `plain_title` TEXT NOT NULL, `plain_snippet` TEXT NOT NULL, `image_url` TEXT, `enclosure_link` TEXT, `author` TEXT, `pub_date` TEXT, `link` TEXT, `unread` INTEGER NOT NULL, `notified` INTEGER NOT NULL, `feed_id` INTEGER, FOREIGN KEY(`feed_id`) REFERENCES `feeds`(`id`) ON UPDATE NO ACTION ON DELETE CASCADE )
            """.trimIndent(),
        )

        database.execSQL(
            """
            INSERT INTO `feed_items_new` (`id`, `guid`, `title`, `plain_title`, `plain_snippet`, `image_url`, `enclosure_link`, `author`, `pub_date`, `link`, `unread`, `notified`, `feed_id`)
            SELECT `id`, `guid`, `title`, `plain_title`, `plain_snippet`, `image_url`, `enclosure_link`, `author`, `pub_date`, `link`, `unread`, `notified`, `feed_id` FROM `feed_items`
            """.trimIndent(),
        )

        // Iterate over all items using the minimum query. Also restrict the text field to
        // 1 MB which should be safe enough considering the window size is 2MB large.
        database.query(
            """
            SELECT id, substr(description,0,1000000) FROM feed_items
            """.trimIndent(),
        ).use { cursor ->
            cursor.forEach {
                val feedItemId = cursor.getLong(0)
                val description = cursor.getString(1)

                @Suppress("DEPRECATION")
                blobOutputStream(feedItemId, FeederApplication.staticFilesDir).bufferedWriter()
                    .use {
                        it.write(description)
                    }
            }
        }

        database.execSQL(
            """
            DROP TABLE feed_items
            """.trimIndent(),
        )

        database.execSQL(
            """
            ALTER TABLE feed_items_new RENAME TO feed_items
            """.trimIndent(),
        )

        database.execSQL(
            """
            CREATE UNIQUE INDEX IF NOT EXISTS `index_feed_items_guid_feed_id` ON `feed_items` (`guid`, `feed_id`)
            """.trimIndent(),
        )

        database.execSQL(
            """
            CREATE INDEX IF NOT EXISTS `index_feed_items_feed_id` ON `feed_items` (`feed_id`)
            """.trimIndent(),
        )

        // And reset response hash on all feeds to trigger parsing of results next sync so items
        // are written disk (in case migration substring was too short)
        database.execSQL(
            """
            UPDATE `feeds` SET `response_hash` = 0
            """.trimIndent(),
        )
    }
}

@Suppress("ktlint:standard:property-naming", "ClassName")
object MIGRATION_8_9 : Migration(8, 9) {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            ALTER TABLE feeds ADD COLUMN response_hash INTEGER NOT NULL DEFAULT 0
            """.trimIndent(),
        )
    }
}

@Suppress("ktlint:standard:property-naming", "ClassName")
object MIGRATION_7_8 : Migration(7, 8) {
    override fun migrate(database: SupportSQLiteDatabase) {
        database.execSQL(
            """
            ALTER TABLE feeds ADD COLUMN last_sync INTEGER NOT NULL DEFAULT 0
            """.trimIndent(),
        )
    }
}

@Suppress("ktlint:standard:property-naming", "ClassName")
object MIGRATION_6_7 : Migration(6, 7) {
    override fun migrate(database: SupportSQLiteDatabase) {
        legacyMigration(database, 6)
    }
}

@Suppress("ktlint:standard:property-naming", "ClassName")
object MIGRATION_5_7 : Migration(5, 7) {
    override fun migrate(database: SupportSQLiteDatabase) {
        legacyMigration(database, 5)
    }
}

private fun legacyMigration(
    database: SupportSQLiteDatabase,
    version: Int,
) {
    // Create new tables and indices
    // Feeds
    database.execSQL(
        """
        CREATE TABLE IF NOT EXISTS `feeds` (`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, `title` TEXT NOT NULL, `custom_title` TEXT NOT NULL, `url` TEXT NOT NULL, `tag` TEXT NOT NULL, `notify` INTEGER NOT NULL, `image_url` TEXT)
        """.trimIndent(),
    )
    database.execSQL(
        """
        CREATE UNIQUE INDEX `index_Feed_url` ON `feeds` (`url`)
        """.trimIndent(),
    )
    database.execSQL(
        """
        CREATE UNIQUE INDEX `index_Feed_id_url_title` ON `feeds` (`id`, `url`, `title`)
        """.trimIndent(),
    )

    // Items
    database.execSQL(
        """
        CREATE TABLE IF NOT EXISTS `feed_items` (`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, `guid` TEXT NOT NULL, `title` TEXT NOT NULL, `description` TEXT NOT NULL, `plain_title` TEXT NOT NULL, `plain_snippet` TEXT NOT NULL, `image_url` TEXT, `enclosure_link` TEXT, `author` TEXT, `pub_date` TEXT, `link` TEXT, `unread` INTEGER NOT NULL, `notified` INTEGER NOT NULL, `feed_id` INTEGER, FOREIGN KEY(`feed_id`) REFERENCES `feeds`(`id`) ON UPDATE NO ACTION ON DELETE CASCADE )
        """.trimIndent(),
    )
    database.execSQL(
        """
        CREATE UNIQUE INDEX `index_feed_item_guid_feed_id` ON `feed_items` (`guid`, `feed_id`)
        """.trimIndent(),
    )
    database.execSQL(
        """
        CREATE  INDEX `index_feed_item_feed_id` ON `feed_items` (`feed_id`)
        """.trimIndent(),
    )

    // Migrate to new tables
    database.query(
        """
        SELECT _id, title, url, tag, customtitle, notify ${if (version == 6) ", imageUrl" else ""}
        FROM Feed
        """.trimIndent(),
    ).use { cursor ->
        cursor.forEach { _ ->
            val oldFeedId = cursor.getLong(0)

            val newFeedId =
                database.insert(
                    "feeds",
                    SQLiteDatabase.CONFLICT_FAIL,
                    contentValues {
                        setString("title" to cursor.getString(1))
                        setString("custom_title" to cursor.getString(4))
                        setString("url" to cursor.getString(2))
                        setString("tag" to cursor.getString(3))
                        setInt("notify" to cursor.getInt(5))
                        if (version == 6) {
                            setString("image_url" to cursor.getString(6))
                        }
                    },
                )

            database.query(
                """
                SELECT title, description, plainTitle, plainSnippet, imageUrl, link, author,
                       pubdate, unread, feed, enclosureLink, notified, guid
                FROM FeedItem
                WHERE feed = $oldFeedId
                """.trimIndent(),
            ).use { cursor ->
                database.inTransaction {
                    cursor.forEach { _ ->
                        database.insert(
                            "feed_items",
                            SQLiteDatabase.CONFLICT_FAIL,
                            contentValues {
                                setString("guid" to cursor.getString(12))
                                setString("title" to cursor.getString(0))
                                setString("description" to cursor.getString(1))
                                setString("plain_title" to cursor.getString(2))
                                setString("plain_snippet" to cursor.getString(3))
                                setString("image_url" to cursor.getString(4))
                                setString("enclosure_link" to cursor.getString(10))
                                setString("author" to cursor.getString(6))
                                setString("pub_date" to cursor.getString(7))
                                setString("link" to cursor.getString(5))
                                setInt("unread" to cursor.getInt(8))
                                setInt("notified" to cursor.getInt(11))
                                setLong("feed_id" to newFeedId)
                            },
                        )
                    }
                }
            }
        }
    }

    // Remove all legacy content
    database.execSQL("DROP TRIGGER IF EXISTS trigger_tag_updater")

    database.execSQL("DROP VIEW IF EXISTS WithUnreadCount")
    database.execSQL("DROP VIEW IF EXISTS TagsWithUnreadCount")

    database.execSQL("DROP TABLE IF EXISTS Feed")
    database.execSQL("DROP TABLE IF EXISTS FeedItem")
}

fun SupportSQLiteDatabase.inTransaction(init: (SupportSQLiteDatabase) -> Unit) {
    beginTransaction()
    try {
        init(this)
        setTransactionSuccessful()
    } finally {
        endTransaction()
    }
}
