package com.nononsenseapps.feeder.model.html

data class LinearTextAnnotation(
    val data: LinearTextAnnotationData,
    /**
     * Inclusive start index
     */
    val start: Int,
    /**
     * Inclusive end index
     */
    var end: Int,
) {
    val endExclusive
        get() = end + 1
}

sealed interface LinearTextAnnotationData

data object LinearTextAnnotationH1 : LinearTextAnnotationData

data object LinearTextAnnotationH2 : LinearTextAnnotationData

data object LinearTextAnnotationH3 : LinearTextAnnotationData

data object LinearTextAnnotationH4 : LinearTextAnnotationData

data object LinearTextAnnotationH5 : LinearTextAnnotationData

data object LinearTextAnnotationH6 : LinearTextAnnotationData

data object LinearTextAnnotationBold : LinearTextAnnotationData

data object LinearTextAnnotationItalic : LinearTextAnnotationData

data object LinearTextAnnotationMonospace : LinearTextAnnotationData

data object LinearTextAnnotationUnderline : LinearTextAnnotationData

data object LinearTextAnnotationStrikethrough : LinearTextAnnotationData

data object LinearTextAnnotationSuperscript : LinearTextAnnotationData

data object LinearTextAnnotationSubscript : LinearTextAnnotationData

data class LinearTextAnnotationFont(
    val face: String,
) : LinearTextAnnotationData

data object LinearTextAnnotationCode : LinearTextAnnotationData

data class LinearTextAnnotationLink(
    val href: String,
) : LinearTextAnnotationData
