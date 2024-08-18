#!/usr/bin/env kotlin
@file:DependsOn("org.jetbrains:markdown-jvm:0.4.1")
@file:DependsOn("net.pwall.mustache:kotlin-mustache:0.10")

import net.pwall.mustache.parser.Parser
import org.intellij.markdown.MarkdownElementTypes
import org.intellij.markdown.ast.ASTNode
import org.intellij.markdown.flavours.commonmark.CommonMarkFlavourDescriptor
import org.intellij.markdown.parser.MarkdownParser
import java.io.File
import java.util.concurrent.TimeUnit

val flavour = CommonMarkFlavourDescriptor()

data class ChangelogEntry(
    val version: String,
    val content: String,
) {
    val timestamp: String
        get() = "git log -1 --format=%aI $version".runCommand()

    val title: String
        get() = "$version released"
}

fun parseChangelog(): List<ChangelogEntry> {
    val src = File("CHANGELOG.md").readText()
    val parsedTree = MarkdownParser(flavour).buildMarkdownTreeFromString(src)
    val entries = mutableListOf<ChangelogEntry>()
    val sb = StringBuilder()

    recurseMarkdown(
        node = parsedTree,
        src = src,
        version = "",
        sb = sb,
        entries = entries,
    )

    return entries
}

fun recurseMarkdown(
    node: ASTNode,
    src: String,
    version: String,
    sb: StringBuilder,
    entries: MutableList<ChangelogEntry>,
): String {
    var newVersion = version
    var ignoreContent = false
    when (node.type) {
        MarkdownElementTypes.MARKDOWN_FILE -> {
            // Keep going directly
            ignoreContent = true
        }

        MarkdownElementTypes.ATX_1 -> {
            // Header marks boundary between entries
            if (sb.isNotBlank()) {
                entries.add(
                    ChangelogEntry(
                        version = newVersion,
                        content = sb.toString(),
                    ),
                )
                sb.clear()
            }
            val textNode = node.children.last()
            return src.slice(textNode.startOffset until textNode.endOffset).trim()
        }
    }

    if (ignoreContent) {
        for (child in node.children) {
            newVersion =
                recurseMarkdown(
                    node = child,
                    src = src,
                    version = newVersion,
                    sb = sb,
                    entries = entries,
                )
        }
    } else {
        val content = src.slice(node.startOffset until node.endOffset)
        sb.append(content)
    }
    return newVersion
}

fun generateHugoEntries(
    targetDir: File,
    entries: List<ChangelogEntry>,
) {
    val hugoTemplateString =
        """
        ---
        title: "{{title}}"
        date: {{timestamp}}
        draft: false
        thumbnail: "feature.png"
        ---
        {{&content}}
        """.trimIndent()

    println("${entries.size} entries")

    val parser = Parser()
    val hugoTemplate = parser.parse(hugoTemplateString)

    entries.forEach { entry ->
        val targetFile = targetDir.resolve("${entry.version}.md")
        if (targetFile.isFile) {
            if (!targetFile.delete()) {
                error("Failed to delete existing file: $targetFile")
            }
        }
        targetDir.resolve("${entry.version}.md").bufferedWriter().use { writer ->
            writer.write(hugoTemplate.processToString(entry))
        }
    }
}

fun String.runCommand(): String {
    val parts = this.split("\\s".toRegex())
    val proc =
        ProcessBuilder(*parts.toTypedArray())
            .directory(File("/home/jonas/workspace/feeder"))
            .redirectOutput(ProcessBuilder.Redirect.PIPE)
            .redirectError(ProcessBuilder.Redirect.PIPE)
            .start()

    proc.waitFor(2, TimeUnit.SECONDS)
    return proc.inputStream.bufferedReader().readText().trim()
}

// $args

val targetDir =
    args.firstOrNull()
        ?.let { File(it) }
        ?: error("Expects target directory as first argument")

if (!targetDir.isDirectory) {
    error("$targetDir does not exist or is not a directory!")
}

// To only generate a specific tag out of the changelog
val tag = args.getOrNull(1)

val entries = parseChangelog()

generateHugoEntries(
    targetDir = targetDir,
    entries =
        entries.filter {
            tag == null || it.version == tag
        },
)
