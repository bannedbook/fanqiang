package com.nononsenseapps.feeder.db.legacy

import android.content.Context
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper
import com.nononsenseapps.feeder.db.room.DATABASE_NAME

const val LEGACY_DATABASE_VERSION = 6
const val LEGACY_DATABASE_NAME = DATABASE_NAME

class LegacyDatabaseHandler constructor(
    context: Context,
    name: String = LEGACY_DATABASE_NAME,
    version: Int = LEGACY_DATABASE_VERSION,
) : SQLiteOpenHelper(context, name, null, version) {
    override fun onCreate(db: SQLiteDatabase) {
    }

    override fun onUpgrade(
        db: SQLiteDatabase,
        oldVersion: Int,
        newVersion: Int,
    ) {
    }

    override fun onOpen(db: SQLiteDatabase) {
        super.onOpen(db)
        if (!db.isReadOnly) {
            // Enable foreign key constraints
            db.setForeignKeyConstraintsEnabled(true)
        }
    }
}

fun createViewsAndTriggers(db: SQLiteDatabase) {
    // Create triggers
    db.execSQL(CREATE_TAG_TRIGGER)
    // Create views if not exists
    db.execSQL(CREATE_COUNT_VIEW)
    db.execSQL(CREATE_TAGS_VIEW)
}

// SQL convention says Table name should be "singular"
const val FEED_TABLE_NAME = "Feed"

// SQL convention says Table name should be "singular"
const val FEED_ITEM_TABLE_NAME = "FeedItem"

// Naming the id column with an underscore is good to be consistent
// with other Android things. This is ALWAYS needed
const val COL_ID = "_id"

// These fields can be anything you want.
const val COL_TITLE = "title"
const val COL_CUSTOM_TITLE = "customtitle"
const val COL_URL = "url"
const val COL_TAG = "tag"
const val COL_NOTIFY = "notify"
const val COL_GUID = "guid"
const val COL_DESCRIPTION = "description"
const val COL_PLAINTITLE = "plainTitle"
const val COL_PLAINSNIPPET = "plainSnippet"
const val COL_IMAGEURL = "imageUrl"
const val COL_ENCLOSURELINK = "enclosureLink"
const val COL_LINK = "link"
const val COL_AUTHOR = "author"
const val COL_PUBDATE = "pubdate"
const val COL_UNREAD = "unread"
const val COL_NOTIFIED = "notified"

// These fields corresponds to columns in Feed table
const val COL_FEED = "feed"
const val COL_FEEDTITLE = "feedtitle"
const val COL_FEEDURL = "feedurl"

const val CREATE_FEED_TABLE = """
    CREATE TABLE $FEED_TABLE_NAME (
      $COL_ID INTEGER PRIMARY KEY,
      $COL_TITLE TEXT NOT NULL,
      $COL_CUSTOM_TITLE TEXT NOT NULL,
      $COL_URL TEXT NOT NULL,
      $COL_TAG TEXT NOT NULL DEFAULT '',
      $COL_NOTIFY INTEGER NOT NULL DEFAULT 0,
      $COL_IMAGEURL TEXT,
      UNIQUE($COL_URL) ON CONFLICT REPLACE
    )"""

const val CREATE_COUNT_VIEW = """
    CREATE TEMP VIEW IF NOT EXISTS WithUnreadCount
    AS SELECT $COL_ID, $COL_TITLE, $COL_URL, $COL_TAG, $COL_CUSTOM_TITLE, $COL_NOTIFY, $COL_IMAGEURL, "unreadcount"
       FROM $FEED_TABLE_NAME
       LEFT JOIN (SELECT COUNT(1) AS ${"unreadcount"}, $COL_FEED
         FROM $FEED_ITEM_TABLE_NAME
         WHERE $COL_UNREAD IS 1
         GROUP BY $COL_FEED)
       ON $FEED_TABLE_NAME.$COL_ID = $COL_FEED"""

const val CREATE_TAGS_VIEW = """
    CREATE TEMP VIEW IF NOT EXISTS TagsWithUnreadCount
    AS SELECT $COL_ID, $COL_TAG, "unreadcount"
       FROM $FEED_TABLE_NAME
       LEFT JOIN (SELECT COUNT(1) AS ${"unreadcount"}, $COL_TAG AS itemtag
         FROM $FEED_ITEM_TABLE_NAME
         WHERE $COL_UNREAD IS 1
         GROUP BY itemtag)
       ON $FEED_TABLE_NAME.$COL_TAG IS itemtag
       GROUP BY $COL_TAG"""

const val CREATE_FEED_ITEM_TABLE = """
    CREATE TABLE $FEED_ITEM_TABLE_NAME (
      $COL_ID INTEGER PRIMARY KEY,
      $COL_GUID TEXT NOT NULL,
      $COL_TITLE TEXT NOT NULL,
      $COL_DESCRIPTION TEXT NOT NULL,
      $COL_PLAINTITLE TEXT NOT NULL,
      $COL_PLAINSNIPPET TEXT NOT NULL,
      $COL_IMAGEURL TEXT,
      $COL_LINK TEXT,
      $COL_ENCLOSURELINK TEXT,
      $COL_AUTHOR TEXT,
      $COL_PUBDATE TEXT,
      $COL_UNREAD INTEGER NOT NULL DEFAULT 1,
      $COL_NOTIFIED INTEGER NOT NULL DEFAULT 0,
      $COL_FEED INTEGER NOT NULL,
      $COL_FEEDTITLE TEXT NOT NULL,
      $COL_FEEDURL TEXT NOT NULL,
      $COL_TAG TEXT NOT NULL,
      FOREIGN KEY($COL_FEED)
        REFERENCES $FEED_TABLE_NAME($COL_ID)
        ON DELETE CASCADE,
      UNIQUE($COL_GUID,$COL_FEED)
        ON CONFLICT IGNORE
    )"""

const val CREATE_TAG_TRIGGER = """
    CREATE TEMP TRIGGER IF NOT EXISTS ${"trigger_tag_updater"}
      AFTER UPDATE OF $COL_TAG,$COL_TITLE
      ON $FEED_TABLE_NAME
      WHEN
        new.$COL_TAG IS NOT old.$COL_TAG
      OR
        new.$COL_TITLE IS NOT old.$COL_TITLE
      BEGIN
        UPDATE $FEED_ITEM_TABLE_NAME
          SET $COL_TAG = new.$COL_TAG,
              $COL_FEEDTITLE = new.$COL_TITLE
          WHERE $COL_FEED IS old.$COL_ID;
      END
"""
