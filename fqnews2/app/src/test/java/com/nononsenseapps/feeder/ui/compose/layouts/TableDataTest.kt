package com.nononsenseapps.feeder.ui.compose.layouts

import org.junit.Test
import kotlin.test.assertEquals

class TableDataTest {
    @Test
    fun `sorting is done correctly`() {
        val tableData =
            TableData.fromCells(
                listOf(
                    TableCell(row = 4, column = 0, rowSpan = 2, colSpan = 2),
                    TableCell(row = 2, column = 0, rowSpan = 2, colSpan = 1),
                    TableCell(row = 1, column = 0, rowSpan = 1, colSpan = 2),
                    TableCell(row = 0, column = 0, rowSpan = 1, colSpan = 1),
                ),
            )

        assertEquals(6, tableData.rows, "Should not have crashed in constructor")
    }
}
