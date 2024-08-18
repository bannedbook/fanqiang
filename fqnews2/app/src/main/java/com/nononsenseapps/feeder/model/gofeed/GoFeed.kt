package com.nononsenseapps.feeder.model.gofeed

/*
	Title           string                   `json:"title,omitempty"`
	Description     string                   `json:"description,omitempty"`
	Link            string                   `json:"link,omitempty"`
	FeedLink        string                   `json:"feedLink,omitempty"`
	Links           []string                 `json:"links,omitempty"`
	Updated         string                   `json:"updated,omitempty"`
	UpdatedParsed   *time.Time               `json:"updatedParsed,omitempty"`
	Published       string                   `json:"published,omitempty"`
	PublishedParsed *time.Time               `json:"publishedParsed,omitempty"`
	Author          *Person                  `json:"author,omitempty"` // Deprecated: Use feed.Authors instead
	Authors         []*Person                `json:"authors,omitempty"`
	Language        string                   `json:"language,omitempty"`
	Image           *Image                   `json:"image,omitempty"`
	Copyright       string                   `json:"copyright,omitempty"`
	Generator       string                   `json:"generator,omitempty"`
	Categories      []string                 `json:"categories,omitempty"`
	DublinCoreExt   *ext.DublinCoreExtension `json:"dcExt,omitempty"`
	ITunesExt       *ext.ITunesFeedExtension `json:"itunesExt,omitempty"`
	Extensions      ext.Extensions           `json:"extensions,omitempty"`
	Custom          map[string]string        `json:"custom,omitempty"`
	Items           []*Item                  `json:"items"`
	FeedType        string                   `json:"feedType"`
	FeedVersion     string                   `json:"feedVersion"`
 */
data class GoFeed(
    val title: String?,
    val description: String?,
    val link: String?,
    val feedLink: String?,
    val links: List<String>?,
    val updated: String?,
    val updatedParsed: String?,
    val published: String?,
    val publishedParsed: String?,
    val author: GoPerson?,
    val authors: List<GoPerson?>?,
    val language: String?,
    val image: GoImage?,
    val copyright: String?,
    val generator: String?,
    val categories: List<String>?,
    val dublinCoreExt: GoDublinCoreExtension?,
    val iTunesExt: GoITunesFeedExtension?,
    val extensions: GoExtensions?,
    val custom: Map<String, String>?,
    val items: List<GoItem?>?,
    val feedType: String?,
    val feedVersion: String?,
)

/*
type Item struct {
	Title           string                   `json:"title,omitempty"`
	Description     string                   `json:"description,omitempty"`
	Content         string                   `json:"content,omitempty"`
	Link            string                   `json:"link,omitempty"`
	Links           []string                 `json:"links,omitempty"`
	Updated         string                   `json:"updated,omitempty"`
	UpdatedParsed   *time.Time               `json:"updatedParsed,omitempty"`
	Published       string                   `json:"published,omitempty"`
	PublishedParsed *time.Time               `json:"publishedParsed,omitempty"`
	Author          *Person                  `json:"author,omitempty"` // Deprecated: Use item.Authors instead
	Authors         []*Person                `json:"authors,omitempty"`
	GUID            string                   `json:"guid,omitempty"`
	Image           *Image                   `json:"image,omitempty"`
	Categories      []string                 `json:"categories,omitempty"`
	Enclosures      []*Enclosure             `json:"enclosures,omitempty"`
	DublinCoreExt   *ext.DublinCoreExtension `json:"dcExt,omitempty"`
	ITunesExt       *ext.ITunesItemExtension `json:"itunesExt,omitempty"`
	Extensions      ext.Extensions           `json:"extensions,omitempty"`
 */
data class GoItem(
    val title: String?,
    val description: String?,
    val content: String?,
    val link: String?,
    val links: List<String>?,
    val updated: String?,
    val updatedParsed: String?,
    val published: String?,
    val publishedParsed: String?,
    val author: GoPerson?,
    val authors: List<GoPerson?>?,
    val guid: String?,
    val image: GoImage?,
    val categories: List<String>?,
    val enclosures: List<GoEnclosure?>?,
    val dublinCoreExt: GoDublinCoreExtension?,
    val iTunesExt: GoITunesItemExtension?,
    val extensions: GoExtensions?,
)

typealias GoExtensions = Map<String, Map<String, List<GoExtension>>>

data class GoPerson(
    val name: String?,
    val email: String?,
)

data class GoImage(
    val url: String?,
    val title: String?,
)

/*
type Enclosure struct {
	URL    string `json:"url,omitempty"`
	Length string `json:"length,omitempty"`
	Type   string `json:"type,omitempty"`
}
 */
data class GoEnclosure(
    val url: String?,
    val length: String?,
    val type: String?,
)

/*
type DublinCoreExtension struct {
	Title       []string `json:"title,omitempty"`
	Creator     []string `json:"creator,omitempty"`
	Author      []string `json:"author,omitempty"`
	Subject     []string `json:"subject,omitempty"`
	Description []string `json:"description,omitempty"`
	Publisher   []string `json:"publisher,omitempty"`
	Contributor []string `json:"contributor,omitempty"`
	Date        []string `json:"date,omitempty"`
	Type        []string `json:"type,omitempty"`
	Format      []string `json:"format,omitempty"`
	Identifier  []string `json:"identifier,omitempty"`
	Source      []string `json:"source,omitempty"`
	Language    []string `json:"language,omitempty"`
	Relation    []string `json:"relation,omitempty"`
	Coverage    []string `json:"coverage,omitempty"`
	Rights      []string `json:"rights,omitempty"`
}
 */
data class GoDublinCoreExtension(
    val title: List<String>?,
    val creator: List<String>?,
    val author: List<String>?,
    val subject: List<String>?,
    val description: List<String>?,
    val publisher: List<String>?,
    val contributor: List<String>?,
    val date: List<String>?,
    val type: List<String>?,
    val format: List<String>?,
    val identifier: List<String>?,
    val source: List<String>?,
    val language: List<String>?,
    val relation: List<String>?,
    val coverage: List<String>?,
    val rights: List<String>?,
)

/*
type ITunesFeedExtension struct {
	Author     string            `json:"author,omitempty"`
	Block      string            `json:"block,omitempty"`
	Categories []*ITunesCategory `json:"categories,omitempty"`
	Explicit   string            `json:"explicit,omitempty"`
	Keywords   string            `json:"keywords,omitempty"`
	Owner      *ITunesOwner      `json:"owner,omitempty"`
	Subtitle   string            `json:"subtitle,omitempty"`
	Summary    string            `json:"summary,omitempty"`
	Image      string            `json:"image,omitempty"`
	Complete   string            `json:"complete,omitempty"`
	NewFeedURL string            `json:"newFeedUrl,omitempty"`
	Type       string            `json:"type,omitempty"`
}
 */
data class GoITunesFeedExtension(
    val author: String?,
    val block: String?,
    val categories: List<GoITunesCategory?>?,
    val explicit: String?,
    val keywords: String?,
    val owner: GoITunesOwner?,
    val subtitle: String?,
    val summary: String?,
    val image: String?,
    val complete: String?,
    val newFeedURL: String?,
    val type: String?,
)

/*
// ITunesItemExtension is a set of extension
// fields for RSS items.
type ITunesItemExtension struct {
	Author            string `json:"author,omitempty"`
	Block             string `json:"block,omitempty"`
	Duration          string `json:"duration,omitempty"`
	Explicit          string `json:"explicit,omitempty"`
	Keywords          string `json:"keywords,omitempty"`
	Subtitle          string `json:"subtitle,omitempty"`
	Summary           string `json:"summary,omitempty"`
	Image             string `json:"image,omitempty"`
	IsClosedCaptioned string `json:"isClosedCaptioned,omitempty"`
	Episode           string `json:"episode,omitempty"`
	Season            string `json:"season,omitempty"`
	Order             string `json:"order,omitempty"`
	EpisodeType       string `json:"episodeType,omitempty"`
}
 */
data class GoITunesItemExtension(
    val author: String?,
    val block: String?,
    val duration: String?,
    val explicit: String?,
    val keywords: String?,
    val subtitle: String?,
    val summary: String?,
    val image: String?,
    val isClosedCaptioned: String?,
    val episode: String?,
    val season: String?,
    val order: String?,
    val episodeType: String?,
)

/*
type ITunesCategory struct {
	Text        string          `json:"text,omitempty"`
	Subcategory *ITunesCategory `json:"subcategory,omitempty"`
}
 */
data class GoITunesCategory(
    val text: String?,
    val subcategory: GoITunesCategory?,
)

/*
type ITunesOwner struct {
	Email string `json:"email,omitempty"`
	Name  string `json:"name,omitempty"`
}
 */
data class GoITunesOwner(
    val email: String?,
    val name: String?,
)

/*
type Extension struct {
	Name     string                 `json:"name"`
	Value    string                 `json:"value"`
	Attrs    map[string]string      `json:"attrs"`
	Children map[string][]Extension `json:"children"`
}
 */
data class GoExtension(
    val name: String?,
    val value: String?,
    val attrs: Map<String, String>?,
    val children: Map<String, List<GoExtension>>?,
)
