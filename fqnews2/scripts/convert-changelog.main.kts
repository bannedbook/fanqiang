#!/usr/bin/env kotlin
@file:DependsOn("org.jetbrains:markdown-jvm:0.4.1")
@file:DependsOn("net.pwall.mustache:kotlin-mustache:0.10")

import net.pwall.mustache.parser.Parser
import org.intellij.markdown.MarkdownElementTypes
import org.intellij.markdown.ast.ASTNode
import org.intellij.markdown.flavours.commonmark.CommonMarkFlavourDescriptor
import org.intellij.markdown.html.HtmlGenerator
import org.intellij.markdown.parser.MarkdownParser
import java.io.File
import java.util.concurrent.TimeUnit

val flavour = CommonMarkFlavourDescriptor()

data class ChangelogEntry(
    val title: String,
    val content: String,
) {
    val id: String
        get() = "https://github.com/spacecowboy/Feeder/blob/master/CHANGELOG.md/$title"
    val link: String
        get() = "https://github.com/spacecowboy/Feeder/blob/master/CHANGELOG.md#${
            title.replace(
                ".",
                "",
            )
        }"

    val timestamp: String
        get() = "git log -1 --format=%aI $title".runCommand()
}

fun parseChangelog(): MutableList<ChangelogEntry> {
    val src = File("CHANGELOG.md").readText()
    val parsedTree = MarkdownParser(flavour).buildMarkdownTreeFromString(src)
    val entries = mutableListOf<ChangelogEntry>()
    val sb = StringBuilder()

    recurseMarkdown(parsedTree, src, "", sb, entries)

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
                        title = newVersion,
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
            newVersion = recurseMarkdown(child, src, newVersion, sb, entries)
        }
    } else {
        val html = HtmlGenerator(src, node, flavour, false).generateHtml()
        sb.append(html)
    }
    return newVersion
}

fun generateAtomChangelog(entries: List<ChangelogEntry>) {
    val atomTemplateString =
        """
        <?xml version='1.0' encoding='UTF-8'?>
        <feed xmlns="http://www.w3.org/2005/Atom" xmlns:media="http://search.yahoo.com/mrss/">
          <id>https://github.com/spacecowboy/Feeder/blob/master/CHANGELOG.md</id>
          <title>Feeder Changelog</title>
          <updated>{{timestamp}}</updated>
          <author>
            <name>Jonas</name>
            <email>banned.ebook@gmail.com</email>
          </author>
          <link rel="self" type="application/atom+xml" href="https://github.com/spacecowboy/Feeder/blob/master/changelog.xml" />
          <link rel="alternate" type="text/html" href="https://github.com/spacecowboy/Feeder/blob/master/CHANGELOG.md" />
          <subtitle>What's new in Feeder</subtitle>
          <icon>https://github.com/spacecowboy/Feeder/blob/master/graphics/f_foreground_512.png?raw=true</icon>
          
          {{#entry}}
          <entry>
              <id>{{id}}</id>
              <link href="{{link}}" rel="alternate" />
              <title>{{title}}</title>
              <updated>{{timestamp}}</updated>
              <author>
                  <name>Jonas</name>
                  <email>banned.ebook@gmail.com</email>
              </author>
              <media:thumbnail url="https://github.com/spacecowboy/Feeder/blob/master/graphics/web_hi_res_512.png?raw=true" />
              
              <content type="html"><![CDATA[ {{&content}} ]]></content>
          </entry>
          {{/entry}}
          
        </feed>
        """.trimIndent()

    val parser = Parser()
    val atomTemplate = parser.parse(atomTemplateString)

    val y =
        atomTemplate.processToString(
            mapOf(
                "timestamp" to entries.first().timestamp,
                "entry" to entries,
            ),
        )

    println(y)
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
val entries = parseChangelog()

generateAtomChangelog(
    entries.filter {
        val numbers = it.title.split(".")
        when {
            numbers.first().toInt() > 2 -> {
                true
            }

            numbers.first().toInt() == 2 -> {
                when {
                    numbers[1].toInt() >= 4 -> {
                        true
                    }

                    else -> {
                        false
                    }
                }
            }

            else -> {
                false
            }
        }
    },
)
