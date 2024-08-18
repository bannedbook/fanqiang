package com.nononsenseapps.feeder.model.html

import androidx.collection.ArrayMap

data class LinearArticle(
    val elements: List<LinearElement>,
)

/**
 * A linear element can contain other linear elements
 */
sealed interface LinearElement

/**
 * Represents a list of items, ordered or unordered
 */
data class LinearList(
    val ordered: Boolean,
    val items: List<LinearListItem>,
) : LinearElement {
    fun isEmpty(): Boolean {
        return items.isEmpty()
    }

    fun isNotEmpty(): Boolean {
        return items.isNotEmpty()
    }

    class Builder(private val ordered: Boolean) {
        private val items: MutableList<LinearListItem> = mutableListOf()

        fun add(item: LinearListItem) {
            items.add(item)
        }

        fun build(): LinearList {
            return LinearList(ordered, items)
        }
    }

    companion object {
        fun build(
            ordered: Boolean,
            block: Builder.() -> Unit,
        ): LinearList {
            return Builder(ordered).apply(block).build()
        }
    }
}

/**
 * Represents a single item in a list
 */
data class LinearListItem(
    val content: List<LinearElement>,
) {
    constructor(block: ListBuilderScope<LinearElement>.() -> Unit) : this(content = ListBuilderScope(block).items)

    constructor(vararg elements: LinearElement) : this(content = elements.toList())

    fun isEmpty(): Boolean {
        return content.isEmpty()
    }

    fun isNotEmpty(): Boolean {
        return content.isNotEmpty()
    }

    class Builder {
        private val content: MutableList<LinearElement> = mutableListOf()

        fun add(element: LinearElement) {
            content.add(element)
        }

        fun build(): LinearListItem {
            return LinearListItem(content)
        }
    }

    companion object {
        fun build(block: Builder.() -> Unit): LinearListItem {
            return Builder().apply(block).build()
        }
    }
}

/**
 * Represents a table
 */
data class LinearTable(
    val rowCount: Int,
    val colCount: Int,
    private val cellsReal: ArrayMap<Coordinate, LinearTableCellItem>,
) : LinearElement {
    val cells: Map<Coordinate, LinearTableCellItem>
        get() = cellsReal

    constructor(
        rowCount: Int,
        colCount: Int,
        cells: List<LinearTableCellItem>,
        leftToRight: Boolean,
    ) : this(
        rowCount,
        colCount,
        ArrayMap<Coordinate, LinearTableCellItem>().apply {
            cells.forEachIndexed { index, item ->
                put(
                    Coordinate(
                        row = index / colCount,
                        col =
                            if (leftToRight) {
                                index % colCount
                            } else {
                                colCount - 1 - index % colCount
                            },
                    ),
                    item,
                )
            }
        },
    )

    fun cellAt(
        row: Int,
        col: Int,
    ): LinearTableCellItem? {
        return cells[Coordinate(row = row, col = col)]
    }

    class Builder(val leftToRight: Boolean) {
        private val cells: ArrayMap<Coordinate, LinearTableCellItem> = ArrayMap()
        private var rowCount: Int = 0
        private var colCount: Int = 0
        private var currentRowColCount = 0
        private var currentRow = 0

        fun add(element: LinearTableCellItem) {
            check(rowCount > 0) { "Must add a row before adding cells" }

            // First find the first empty cell in this row
            var cellCoord = Coordinate(row = currentRow, col = currentRowColCount)
            while (cells[cellCoord] != null) {
                currentRowColCount++
                cellCoord = cellCoord.copy(col = currentRowColCount)
            }

            currentRowColCount += element.colSpan
            if (currentRowColCount > colCount) {
                colCount = currentRowColCount
            }

            cells[cellCoord] = element

            // Insert filler elements for spanned cells
            for (r in 0 until element.rowSpan) {
                for (c in 0 until element.colSpan) {
                    // Skip first since this is the cell itself
                    if (r == 0 && c == 0) {
                        continue
                    }

                    val fillerCoord = Coordinate(row = currentRow + r, col = currentRowColCount - element.colSpan + c)
                    check(cells[fillerCoord] == null) { "Cell at filler $fillerCoord already exists" }
                    cells[fillerCoord] = LinearTableCellItem.filler
                }
            }
        }

        fun newRow() {
            if (rowCount > 0) {
                currentRow++
            }
            rowCount++
            currentRowColCount = 0
        }

        fun build(): LinearTable {
            return LinearTable(
                rowCount = rowCount,
                colCount = colCount,
                cellsReal =
                    if (leftToRight) {
                        cells
                    } else {
                        ArrayMap<Coordinate, LinearTableCellItem>().apply {
                            cells.forEach { (ltrCoord, item) ->
                                put(
                                    Coordinate(
                                        row = ltrCoord.row,
                                        col = colCount - 1 - ltrCoord.col,
                                    ),
                                    item,
                                )
                            }
                        }
                    },
            )
        }
    }

    companion object {
        fun build(
            leftToRight: Boolean,
            block: Builder.() -> Unit,
        ): LinearTable {
            return Builder(leftToRight = leftToRight).apply(block).build()
        }
    }
}

data class Coordinate(
    val row: Int,
    val col: Int,
)

/**
 * Represents a single cell in a table
 */
data class LinearTableCellItem(
    val type: LinearTableCellItemType,
    val colSpan: Int,
    val rowSpan: Int,
    val content: List<LinearElement>,
) {
    constructor(
        colSpan: Int,
        rowSpan: Int,
        type: LinearTableCellItemType,
        block: ListBuilderScope<LinearElement>.() -> Unit,
    ) : this(colSpan = colSpan, rowSpan = rowSpan, type = type, content = ListBuilderScope(block).items)

    val isFiller
        get() = colSpan == filler.colSpan && rowSpan == filler.rowSpan

    class Builder(
        private val colSpan: Int,
        private val rowSpan: Int,
        private val type: LinearTableCellItemType,
    ) {
        private val content: MutableList<LinearElement> = mutableListOf()

        fun add(element: LinearElement) {
            content.add(element)
        }

        fun build(): LinearTableCellItem {
            return LinearTableCellItem(colSpan = colSpan, rowSpan = rowSpan, type = type, content = content)
        }
    }

    companion object {
        fun build(
            colSpan: Int,
            rowSpan: Int,
            type: LinearTableCellItemType,
            block: Builder.() -> Unit,
        ): LinearTableCellItem {
            return Builder(colSpan = colSpan, rowSpan = rowSpan, type = type).apply(block).build()
        }

        val filler =
            LinearTableCellItem(
                type = LinearTableCellItemType.DATA,
                colSpan = -1,
                rowSpan = -1,
                content = emptyList(),
            )
    }
}

enum class LinearTableCellItemType {
    HEADER,
    DATA,
}

data class LinearBlockQuote(
    val cite: String?,
    val content: List<LinearElement>,
) : LinearElement {
    constructor(cite: String?, block: ListBuilderScope<LinearElement>.() -> Unit) : this(cite = cite, content = ListBuilderScope(block).items)

    constructor(cite: String?, vararg elements: LinearElement) : this(cite = cite, content = elements.toList())
}

/**
 * Primitives can not contain other elements
 */
sealed interface LinearPrimitive : LinearElement

/**
 * Represents a text element. For example a paragraph, or a header.
 */
data class LinearText(
    val text: String,
    val annotations: List<LinearTextAnnotation>,
    val blockStyle: LinearTextBlockStyle,
) : LinearPrimitive {
    constructor(text: String, blockStyle: LinearTextBlockStyle, vararg annotations: LinearTextAnnotation) : this(text = text, blockStyle = blockStyle, annotations = annotations.toList())
}

enum class LinearTextBlockStyle {
    TEXT,
    PRE_FORMATTED,
    CODE_BLOCK,
}

val LinearTextBlockStyle.shouldSoftWrap: Boolean
    get() = this == LinearTextBlockStyle.TEXT

/**
 * Represents an image element
 */
data class LinearImage(
    val sources: List<LinearImageSource>,
    val caption: LinearText?,
    val link: String?,
) : LinearElement

data class LinearImageSource(
    val imgUri: String,
    val widthPx: Int?,
    val heightPx: Int?,
    val pixelDensity: Float?,
    val screenWidth: Int?,
) {
    init {
        if (widthPx != null && widthPx < 1) {
            throw IllegalArgumentException("Width must be positive: $widthPx")
        }
        if (heightPx != null && heightPx < 1) {
            throw IllegalArgumentException("Height must be positive: $heightPx")
        }
        if (pixelDensity != null && pixelDensity <= 0) {
            throw IllegalArgumentException("Pixel density must be positive: $pixelDensity")
        }
        if (screenWidth != null && screenWidth < 1) {
            throw IllegalArgumentException("Screen width must be positive: $screenWidth")
        }
    }
}

/**
 * Represents a video element
 */
data class LinearVideo(
    val sources: List<LinearVideoSource>,
) : LinearElement {
    init {
        require(sources.isNotEmpty()) { "At least one source must be provided" }
    }

    val imageThumbnail: String? by lazy {
        sources.firstOrNull { it.imageThumbnail != null }?.imageThumbnail
    }

    val firstSource: LinearVideoSource
        get() = sources.first()
}

data class LinearVideoSource(
    val uri: String,
    // This might be different from the uri, for example for youtube videos where uri is the embed uri
    val link: String,
    val imageThumbnail: String?,
    val widthPx: Int?,
    val heightPx: Int?,
    val mimeType: String?,
) {
    init {
        if (widthPx != null && widthPx < 1) {
            throw IllegalArgumentException("Width must be positive: $widthPx")
        }
        if (heightPx != null && heightPx < 1) {
            throw IllegalArgumentException("Height must be positive: $heightPx")
        }
    }
}

/**
 * Represents an audio element
 */
data class LinearAudio(
    val sources: List<LinearAudioSource>,
) : LinearElement {
    init {
        require(sources.isNotEmpty()) { "At least one source must be provided" }
    }

    val firstSource: LinearAudioSource
        get() = sources.first()
}

data class LinearAudioSource(
    val uri: String,
    val mimeType: String?,
)
