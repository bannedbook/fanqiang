package com.nononsenseapps.text;

import com.nononsenseapps.feeder.ui.text.HtmlToPlainTextConverter;
import com.nononsenseapps.feeder.ui.text.HtmlToPlainTextConverterKt;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class HtmlToPlainTextConverterTest {
    @Test
    public void repeated() throws Exception {
        assertEquals("", HtmlToPlainTextConverterKt.repeated("*", 0));
        assertEquals("", HtmlToPlainTextConverterKt.repeated("*", -1));
        assertEquals("*", HtmlToPlainTextConverterKt.repeated("*", 1));
        assertEquals("****", HtmlToPlainTextConverterKt.repeated("*", 4));
    }

    @Test
    public void empty() throws Exception {
        assertEquals("", new HtmlToPlainTextConverter().convert(""));
    }

    @Test
    public void unorderedList() throws Exception {
        assertEquals("* one\n" +
                "* two", new HtmlToPlainTextConverter().convert("<ul><li>one</li><li>two</li></ul>"));
    }

    @Test
    public void orderedList() throws Exception {
        assertEquals("1. one\n" +
                "2. two", new HtmlToPlainTextConverter().convert("<ol><li>one</li><li>two</li></ol>"));
    }

    @Test
    public void nestedList() throws Exception {
        assertEquals("1. sublist:\n" +
                "  * A\n" +
                "  * B\n" +
                "2. two",
                new HtmlToPlainTextConverter().convert("<ol><li>sublist:<ul><li>A</li><li>B</li></ul></li><li>two</li></ol>"));
    }

    @Test
    public void link() throws Exception {
        assertEquals("go to Google and see.",
                new HtmlToPlainTextConverter().convert("go to <a href=\"google.com\">Google</a> and see."));
    }

    @Test
    public void noNewLines() throws Exception {
        assertEquals("one two three", new HtmlToPlainTextConverter().convert("<p>one<br>two<p>three"));
    }

    @Test
    public void noScripts() throws Exception {
        assertEquals("foo bar",
                new HtmlToPlainTextConverter().convert("foo <script>script</script> bar"));
    }

    @Test
    public void noStyles() throws Exception {
        assertEquals("foo bar",
                new HtmlToPlainTextConverter().convert("foo <style>style</style> bar"));
    }

    @Test
    public void imgSkipped() throws Exception {
        assertEquals("foo bar",
                new HtmlToPlainTextConverter().convert("foo <img src=\"meh\" alt=\"meh\"> bar"));
    }

    @Test
    public void noBold() throws Exception {
        assertEquals("foo", new HtmlToPlainTextConverter().convert("<b>foo</b>"));
    }

    @Test
    public void noItalic() throws Exception {
        assertEquals("foo", new HtmlToPlainTextConverter().convert("<i>foo</i>"));
    }

    @Test
    public void noLeadingImages() throws Exception {
        assertEquals("foo", new HtmlToPlainTextConverter().convert("<img src='bar' alt='heh'> foo"));
    }
}
