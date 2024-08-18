package io.nekohasekai.sagernet.fmt.http

import io.nekohasekai.sagernet.fmt.v2ray.isTLS
import io.nekohasekai.sagernet.fmt.v2ray.setTLS
import io.nekohasekai.sagernet.ktx.urlSafe
import okhttp3.HttpUrl
import okhttp3.HttpUrl.Companion.toHttpUrlOrNull

fun parseHttp(link: String): HttpBean {
    val httpUrl = link.toHttpUrlOrNull() ?: error("Invalid http(s) link: $link")

    if (httpUrl.encodedPath != "/") error("Not http proxy")

    return HttpBean().apply {
        serverAddress = httpUrl.host
        serverPort = httpUrl.port
        username = httpUrl.username
        password = httpUrl.password
        sni = httpUrl.queryParameter("sni")
        name = httpUrl.fragment
        setTLS(httpUrl.scheme == "https")
    }
}

fun HttpBean.toUri(): String {
    val builder = HttpUrl.Builder().scheme(if (isTLS()) "https" else "http").host(serverAddress)

    if (serverPort in 1..65535) {
        builder.port(serverPort)
    }

    if (username.isNotBlank()) {
        builder.username(username)
    }
    if (password.isNotBlank()) {
        builder.password(password)
    }
    if (sni.isNotBlank()) {
        builder.addQueryParameter("sni", sni)
    }
    if (name.isNotBlank()) {
        builder.encodedFragment(name.urlSafe())
    }

    return builder.toString()
}