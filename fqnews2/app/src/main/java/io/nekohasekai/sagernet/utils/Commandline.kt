package io.nekohasekai.sagernet.utils

import java.util.*

/**
 * Commandline objects help handling command lines specifying processes to
 * execute.
 *
 * The class can be used to define a command line as nested elements or as a
 * helper to define a command line by an application.
 *
 *
 * `
 * <someelement><br></br>
 * &nbsp;&nbsp;<acommandline executable="/executable/to/run"><br></br>
 * &nbsp;&nbsp;&nbsp;&nbsp;<argument value="argument 1" /><br></br>
 * &nbsp;&nbsp;&nbsp;&nbsp;<argument line="argument_1 argument_2 argument_3" /><br></br>
 * &nbsp;&nbsp;&nbsp;&nbsp;<argument value="argument 4" /><br></br>
 * &nbsp;&nbsp;</acommandline><br></br>
 * </someelement><br></br>
` *
 *
 * Based on: https://github.com/apache/ant/blob/588ce1f/src/main/org/apache/tools/ant/types/Commandline.java
 *
 * Adds support for escape character '\'.
 */
object Commandline {

    /**
     * Quote the parts of the given array in way that makes them
     * usable as command line arguments.
     * @param args the list of arguments to quote.
     * @return empty string for null or no command, else every argument split
     * by spaces and quoted by quoting rules.
     */
    fun toString(args: Iterable<String>?): String {
        // empty path return empty string
        args ?: return ""
        // path containing one or more elements
        val result = StringBuilder()
        for (arg in args) {
            if (result.isNotEmpty()) result.append(' ')
            arg.indices.map { arg[it] }.forEach {
                when (it) {
                    ' ', '\\', '"', '\'' -> {
                        result.append('\\')  // intentionally no break
                        result.append(it)
                    }
                    else -> result.append(it)
                }
            }
        }
        return result.toString()
    }

    /**
     * Quote the parts of the given array in way that makes them
     * usable as command line arguments.
     * @param args the list of arguments to quote.
     * @return empty string for null or no command, else every argument split
     * by spaces and quoted by quoting rules.
     */
    fun toString(args: Array<String>) =
        toString(args.asIterable()) // thanks to Java, arrays aren't iterable

    /**
     * Crack a command line.
     * @param toProcess the command line to process.
     * @return the command line broken into strings.
     * An empty or null toProcess parameter results in a zero sized array.
     */
    fun translateCommandline(toProcess: String?): Array<String> {
        if (toProcess == null || toProcess.isEmpty()) {
            //no command? no string
            return arrayOf()
        }
        // parse with a simple finite state machine

        val normal = 0
        val inQuote = 1
        val inDoubleQuote = 2
        var state = normal
        val tok = StringTokenizer(toProcess, "\\\"\' ", true)
        val result = ArrayList<String>()
        val current = StringBuilder()
        var lastTokenHasBeenQuoted = false
        var lastTokenIsSlash = false

        while (tok.hasMoreTokens()) {
            val nextTok = tok.nextToken()
            when (state) {
                inQuote -> if ("\'" == nextTok) {
                    lastTokenHasBeenQuoted = true
                    state = normal
                } else current.append(nextTok)
                inDoubleQuote -> when (nextTok) {
                    "\"" -> if (lastTokenIsSlash) {
                        current.append(nextTok)
                        lastTokenIsSlash = false
                    } else {
                        lastTokenHasBeenQuoted = true
                        state = normal
                    }
                    "\\" -> lastTokenIsSlash = if (lastTokenIsSlash) {
                        current.append(nextTok)
                        false
                    } else true
                    else -> {
                        if (lastTokenIsSlash) {
                            current.append("\\")   // unescaped
                            lastTokenIsSlash = false
                        }
                        current.append(nextTok)
                    }
                }
                else -> {
                    when {
                        lastTokenIsSlash -> {
                            current.append(nextTok)
                            lastTokenIsSlash = false
                        }
                        "\\" == nextTok -> lastTokenIsSlash = true
                        "\'" == nextTok -> state = inQuote
                        "\"" == nextTok -> state = inDoubleQuote
                        " " == nextTok -> if (lastTokenHasBeenQuoted || current.isNotEmpty()) {
                            result.add(current.toString())
                            current.setLength(0)
                        }
                        else -> current.append(nextTok)
                    }
                    lastTokenHasBeenQuoted = false
                }
            }
        }
        if (lastTokenHasBeenQuoted || current.isNotEmpty()) result.add(current.toString())
        require(state != inQuote && state != inDoubleQuote) { "unbalanced quotes in $toProcess" }
        require(!lastTokenIsSlash) { "escape character following nothing in $toProcess" }
        return result.toTypedArray()
    }
}
