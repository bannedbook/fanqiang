#!/bin/bash -e

# Actually using spaRSS db
# v1 db file
V1="FeedEx.db"
# v2 db file
V2="db.empty"

OUT="v2"

echo destroying "$OUT"
> "$OUT"

sqlite3 "$V2" .dump |  sqlite3 "$OUT"
# a small cleanup
sqlite3 "$OUT" "
	drop index index_feeds_groupId;
	drop index index_feeds_feedId_feedLink;
	drop index index_entries_feedId;
	drop index index_entries_link;
	drop index index_tasks_entryId;
	drop trigger feed_insert_priority;
"
sqlite3 "$V1" .dump | \
	grep -A999999 "CREATE TABLE feeds" | \
	grep -B999999 "CREATE TABLE tasks" | head -n -1 | \
	sed -E 's/^CREATE TABLE feeds/CREATE TABLE feeds_v1/' | \
	sed -E 's/^INSERT INTO "?feeds"?/INSERT INTO feeds_v1/' | \
	sed -E 's/^CREATE TABLE entries/CREATE TABLE entries_v1/' | \
	sed -E 's/^INSERT INTO "?entries"?/INSERT INTO entries_v1/' | \
	sqlite3 "$OUT"
sqlite3 "$OUT" "drop table filters;"

sqlite3 "$OUT" "delete from feeds;"
# left out: feedImageLink, 
sqlite3 "$OUT" "insert into feeds (
	feedId,	feedLink,	feedTitle,	fetchError,	retrieveFullText,
	isGroup,		groupId,	displayPriority,	lastManualActionUid)
select
	_id,		ifnull(url,''),	name,		'',			ifnull(fetchmode,0),
	ifnull(isgroup,0),	groupid,	priority,			ifnull(reallastupdate,'')
from feeds_v1
;"
sqlite3 "$OUT" "drop table feeds_v1;"


# UNIQUE-ify entries_v1.link:
sqlite3 "$OUT" "delete from entries_v1
	where _id not in
	( select min(_id) from entries_v1 group by link );"

sqlite3 "$OUT" "delete from entries;"
sqlite3 "$OUT" "insert into entries (
	id,	feedId,	link,	fetchDate,	publicationDate,	title,	description,
	mobilizedContent,	imageLink,	author,	read,	favorite)
select
	_id,	feedid,	link,	ifnull(fetch_date,''),	date,	title,	abstract,
	mobilized,			image_url,	author,	ifnull(isread,0),	ifnull(favorite,0)
from entries_v1
;"
sqlite3 "$OUT" "drop table entries_v1;"


echo destroying "$OUT".new
> "$OUT".new
sqlite3 "$OUT" .dump | sqlite3 "$OUT".new && rm "$OUT"

