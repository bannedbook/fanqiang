package io.nekohasekai.sagernet.bg.proto

import io.nekohasekai.sagernet.database.DataStore
import io.nekohasekai.sagernet.database.ProxyEntity

class UrlTest {

    val link = DataStore.connectionTestURL
    val timeout = 3000

    suspend fun doTest(profile: ProxyEntity): Int {
        return TestInstance(profile, link, timeout).doTest()
    }

}