package com.nononsenseapps.feeder.util

import android.content.Intent.ACTION_SENDTO
import android.content.Intent.ACTION_VIEW
import android.content.Intent.EXTRA_EMAIL
import android.content.Intent.EXTRA_SUBJECT
import android.content.Intent.EXTRA_TEXT
import android.net.Uri
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.filters.MediumTest
import app.Constants.CRASH_REPORT_ADDRESS
import app.Constants.EMAIL_REPORT_ADDRESS
import org.junit.Test
import org.junit.runner.RunWith
import kotlin.test.assertEquals
import kotlin.test.assertTrue

@RunWith(AndroidJUnit4::class)
@MediumTest
class BugReportKTest {
    @Test
    fun bugIntentIsCorrect() {
        val intent = emailBugReportIntent()

        assertEquals(ACTION_SENDTO, intent.action)
        assertEquals(Uri.parse("mailto:$EMAIL_REPORT_ADDRESS"), intent.data)
        assertEquals("Bug report for Feeder", intent.getStringExtra(EXTRA_SUBJECT))
        assertEquals(EMAIL_REPORT_ADDRESS, intent.getStringExtra(EXTRA_EMAIL))

        assertTrue(intent.getStringExtra(EXTRA_TEXT)) {
            "Application: " in intent.getStringExtra(EXTRA_TEXT)!!
        }
    }

    @Test
    fun crashIntentIsCorrect() {
        try {
            @Suppress("DIVISION_BY_ZERO")
            1 / 0
        } catch (e: Exception) {
            val intent = emailCrashReportIntent(e)

            assertEquals(ACTION_SENDTO, intent.action)
            assertEquals(Uri.parse("mailto:$CRASH_REPORT_ADDRESS"), intent.data)
            assertEquals("Crash report for Feeder", intent.getStringExtra(EXTRA_SUBJECT))
            assertEquals(CRASH_REPORT_ADDRESS, intent.getStringExtra(EXTRA_EMAIL))

            assertTrue(intent.getStringExtra(EXTRA_TEXT)) {
                "java.lang.ArithmeticException: divide by zero" in intent.getStringExtra(EXTRA_TEXT)!!
            }
        }
    }

    @Test
    fun issuesIntentIsCorrect() {
        val intent = openGitlabIssues()

        assertEquals(ACTION_VIEW, intent.action)
        assertEquals(Uri.parse("https://gitlab.com/spacecowboy/Feeder/issues"), intent.data)
    }
}
