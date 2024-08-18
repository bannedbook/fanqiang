pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
        flatDir {
            dirs("libcore") // 指定查找 .aar 文件的位置
        }
    }
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
        maven { url = uri("https://jitpack.io") }
        mavenLocal()
    }

    versionCatalogs {
        create("libs") {
            // Compose compiler is highly coupled to Kotlin version
            // See https://developer.android.com/jetpack/androidx/releases/compose-kotlin#pre-release_kotlin_compatibility
            val kotlinVersion = "1.9.23"
            val kspVersion = "1.9.23-1.0.20"
            val kotlinxSerialization = "1.6.1"
            version("kotlin", kotlinVersion)
            version("ksp", kspVersion)
            version("kotlinxSerialization", kotlinxSerialization)
            version("androidPlugin", "8.3.0")
            version("composeCompiler", "1.5.11")

            // BEGIN These should be upgraded in unison
            version("okhttp", "4.10.0")
            val okioVersion = "3.2.0"
            version("okio", okioVersion)
            version("conscrypt", "2.5.2")
            // END Unison

            // Rest
            version("ktlint-gradle", "12.1.1")
            version("ktlint-compose", "0.4.3")
            version("kodein", "7.5.0")
            version("coroutines", "1.7.3")
            version("gofeed", "0.1.2")
            version("moshi", "1.12.0")
            version("desugar", "2.0.3")
            version("jsoup", "1.7.3")
            version("tagsoup", "1.2.1")
            version("readability4j", "1.0.5")
            version("retrofit", "2.9.0")
            version("qrgen", "2.6.0")
            version("androidxCore", "1.10.1")
            version("androidxTestcore", "1.5.0")
            version("workmanager", "2.9.1")
            version("appcompat", "1.6.1")
            version("material", "1.6.1")
            version("preference", "1.2.1")
            version("testRunner", "1.4.0")
            version("lifecycle", "2.6.2")
            version("room", "2.5.2")
            // Compose related below
            version("compose", "2024.04.00")
            val activityCompose = "1.7.0"
            version("paging", "3.2.1")
            version("accompanist", "0.30.1")
            version("coil", "2.4.0")
            version("androidWindow", "1.0.0")
            // Formerly customtabs
            version("androidxBrowser", "1.5.0")
            // Tests
            version("junit", "4.13.2")
            version("espresso", "3.3.0")
            version("mockk", "1.13.3")
            version("mockito", "2.13.0")
            version("androidx-test-junit-ktx", "1.1.4")

            // Plugins
            plugin("android-application", "com.android.application").versionRef("androidPlugin")
            plugin("kotlin-android", "org.jetbrains.kotlin.android").versionRef("kotlin")
            plugin("kotlin-jvm", "org.jetbrains.kotlin.jvm").versionRef("kotlin")
            plugin("kotlin-ksp", "com.google.devtools.ksp").versionRef("ksp")
            plugin("kotlin-parcelize", "org.jetbrains.kotlin.plugin.parcelize").versionRef("kotlin")
            plugin("kotlin-serialization", "org.jetbrains.kotlin.plugin.serialization").versionRef("kotlin")
            plugin("ktlint-gradle", "org.jlleitschuh.gradle.ktlint").versionRef("ktlint-gradle")

            // BOMS
            library("okhttp-bom", "com.squareup.okhttp3", "okhttp-bom").versionRef("okhttp")
            library("coil-bom", "io.coil-kt", "coil-bom").versionRef("coil")
            library("compose-bom", "androidx.compose", "compose-bom").versionRef("compose")

            // Libraries
            library("ktlint-compose", "io.nlopez.compose.rules", "ktlint").versionRef("ktlint-compose")
            library("room", "androidx.room", "room-compiler").versionRef("room")
            library("room-ktx", "androidx.room", "room-ktx").versionRef("room")
            library("room-paging", "androidx.room", "room-paging").versionRef("room")

            library(
                "work-runtime-ktx",
                "androidx.work",
                "work-runtime-ktx",
            ).versionRef("workmanager")

            library("core-ktx", "androidx.core", "core-ktx").versionRef("androidxCore")
            library("androidx-appcompat", "androidx.appcompat", "appcompat").versionRef("appcompat")

            library(
                "androidx-preference",
                "androidx.preference",
                "preference",
            ).versionRef("preference")

            // ViewModel
            library(
                "lifecycle-runtime-compose",
                "androidx.lifecycle",
                "lifecycle-runtime-compose",
            ).versionRef("lifecycle")
            library(
                "lifecycle-runtime-ktx",
                "androidx.lifecycle",
                "lifecycle-runtime-ktx",
            ).versionRef("lifecycle")
            library(
                "lifecycle-viewmodel-ktx",
                "androidx.lifecycle",
                "lifecycle-viewmodel-ktx",
            ).versionRef("lifecycle")
            library(
                "lifecycle-viewmodel-savedstate",
                "androidx.lifecycle",
                "lifecycle-viewmodel-savedstate",
            ).versionRef("lifecycle")
            library(
                "paging-runtime-ktx",
                "androidx.paging",
                "paging-runtime-ktx",
            ).versionRef("paging")

            // Compose
            // Overriding this to newer version than BOM because of predictive back
            library(
                "activity-compose",
                "androidx.activity",
                "activity-compose",
            ).version {
                require(activityCompose)
            }
            library("ui", "androidx.compose.ui", "ui").withoutVersion()
            library("foundation", "androidx.compose.foundation", "foundation").withoutVersion()
            library(
                "foundation-layout",
                "androidx.compose.foundation",
                "foundation-layout",
            ).withoutVersion()
            library("compose-material3", "androidx.compose.material3", "material3")
                .version {
                // 1.1.0 introduced tooltips, not part of compose 05 bom at least
                require("1.1.0")
            }
            library("compose-material", "androidx.compose.material", "material").withoutVersion()
            library(
                "compose-material3-windowsizeclass",
                "androidx.compose.material3",
                "material3-window-size-class",
            ).withoutVersion()
            library(
                "compose-material-icons-extended",
                "androidx.compose.material",
                "material-icons-extended",
            ).withoutVersion()
            library("runtime", "androidx.compose.runtime", "runtime").withoutVersion()
            library("ui-tooling", "androidx.compose.ui", "ui-tooling").withoutVersion()
            library(
                "navigation-compose",
                "androidx.navigation",
                "navigation-compose",
            ).version("2.7.7")
            library(
                "paging-compose",
                "androidx.paging",
                "paging-compose",
            ).versionRef("paging")
            library("window", "androidx.window", "window").versionRef("androidWindow")
            library(
                "android-material",
                "com.google.android.material",
                "material",
            ).versionRef("material")
            library(
                "accompanist-permissions",
                "com.google.accompanist",
                "accompanist-permissions",
            ).versionRef("accompanist")
            library(
                "accompanist-systemuicontroller",
                "com.google.accompanist",
                "accompanist-systemuicontroller",
            ).versionRef("accompanist")
            library(
                "accompanist-adaptive",
                "com.google.accompanist",
                "accompanist-adaptive",
            ).versionRef("accompanist")

            // Better times
            library("desugar", "com.android.tools", "desugar_jdk_libs").versionRef("desugar")
            // HTML parsing
            library("jsoup", "org.jsoup", "jsoup").versionRef("jsoup")
            library("tagsoup", "org.ccil.cowan.tagsoup", "tagsoup").versionRef("tagsoup")
            // RSS
            //library("gofeed-android", "com.nononsenseapps.gofeed", "gofeed-android").versionRef("gofeed")

            // For better fetching
            library("okhttp", "com.squareup.okhttp3", "okhttp").withoutVersion()
            library("okio", "com.squareup.okio", "okio").version {
                strictly(okioVersion)
            }
            // For supporting TLSv1.3 on pre Android-10
            library(
                "conscrypt-android",
                "org.conscrypt",
                "conscrypt-android",
            ).versionRef("conscrypt")

            // Image loading
            library("coil-base", "io.coil-kt", "coil-base").withoutVersion()
            library("coil-gif", "io.coil-kt", "coil-gif").withoutVersion()
            library("coil-svg", "io.coil-kt", "coil-svg").withoutVersion()
            library("coil-compose", "io.coil-kt", "coil-compose").withoutVersion()

            library("kotlin-stdlib", "org.jetbrains.kotlin", "kotlin-stdlib").version {
                strictly(kotlinVersion)
            }
            library(
                "kotlin-stdlib-common",
                "org.jetbrains.kotlin",
                "kotlin-stdlib-common",
            ).version {
                strictly(kotlinVersion)
            }
            library("kotlin-serialization-json", "org.jetbrains.kotlinx", "kotlinx-serialization-json").versionRef("kotlinxSerialization")
            library(
                "kotlin-test-junit",
                "org.jetbrains.kotlin",
                "kotlin-test-junit",
            ).versionRef("kotlin")
            // Coroutines
            library(
                "kotlin-coroutines-test",
                "org.jetbrains.kotlinx",
                "kotlinx-coroutines-test",
            ).versionRef("coroutines")
            library(
                "kotlin-coroutines-core",
                "org.jetbrains.kotlinx",
                "kotlinx-coroutines-core",
            ).versionRef("coroutines")
            // For doing coroutines on UI thread
            library(
                "kotlin-coroutines-android",
                "org.jetbrains.kotlinx",
                "kotlinx-coroutines-android",
            ).versionRef("coroutines")
            // Dependency injection
            library(
                "kodein-androidx",
                "org.kodein.di",
                "kodein-di-framework-android-x",
            ).versionRef("kodein")
            // Custom tabs
            library("androidx-browser", "androidx.browser", "browser").versionRef("androidxBrowser")
            // Full text
            library(
                "readability4j",
                "net.dankito.readability4j",
                "readability4j",
            ).versionRef("readability4j")
            // For feeder-sync
            library("retrofit", "com.squareup.retrofit2", "retrofit").versionRef("retrofit")
            library(
                "retrofit-converter-moshi",
                "com.squareup.retrofit2",
                "converter-moshi",
            ).versionRef("retrofit")
            library("moshi", "com.squareup.moshi", "moshi").versionRef("moshi")
            library("moshi-kotlin", "com.squareup.moshi", "moshi-kotlin").versionRef("moshi")
            library("moshi-adapters", "com.squareup.moshi", "moshi-adapters").versionRef("moshi")
            library("qrgen", "com.github.kenglxn.qrgen", "android").versionRef("qrgen")

            // Feel free to upgrade once we move to later sdk
            // Only necessary to fix a bad transitive dependency by Google
            library("emoji2-view-helper", "androidx.emoji2", "emoji2-views-helper").version {
                strictly("1.3.+")
            }
            library("emoji2", "androidx.emoji2", "emoji2").version {
                strictly("1.3.+")
            }

            // testing
            library("junit", "junit", "junit").versionRef("junit")
            library("mockito-core", "org.mockito", "mockito-core").versionRef("mockito")
            library("mockk", "io.mockk", "mockk").versionRef("mockk")
            library("mockwebserver", "com.squareup.okhttp3", "mockwebserver").versionRef("okhttp")

            library("mockk-android", "io.mockk", "mockk-android").versionRef("mockk")
            library("androidx-test-core", "androidx.test", "core").versionRef("androidxTestcore")
            library(
                "androidx-test-core-ktx",
                "androidx.test",
                "core-ktx",
            ).versionRef("androidxTestcore")
            library("androidx-test-runner", "androidx.test", "runner").versionRef("testRunner")
            library("room-testing", "androidx.room", "room-testing").versionRef("room")
            library(
                "espresso-core",
                "androidx.test.espresso",
                "espresso-core",
            ).versionRef("espresso")
            library(
                "compose-ui-test-junit4",
                "androidx.compose.ui",
                "ui-test-junit4",
            ).withoutVersion()
            library(
                "compose-ui-test-manifest",
                "androidx.compose.ui",
                "ui-test-manifest",
            ).withoutVersion()
            library(
                "androidx-test-junit-ktx",
                "androidx.test.ext",
                "junit-ktx",
            ).versionRef("androidx-test-junit-ktx")

            // bundles
            bundle("okhttp", listOf("okhttp", "okio"))
            bundle("okhttp-android", listOf("okhttp", "okio", "conscrypt-android"))
            bundle(
                "kotlin",
                listOf(
                    "kotlin-stdlib",
                    "kotlin-stdlib-common",
                    "kotlin-coroutines-core",
                    "kotlin-serialization-json",
                ),
            )

            bundle(
                "jvm",
                listOf(
                    "jsoup",
                    "tagsoup",
                    "readability4j",
                    "retrofit",
                    "retrofit-converter-moshi",
                    "moshi",
                    "moshi-kotlin",
                    "moshi-adapters",
                    "qrgen",
                    //"gofeed-android"
                ),
            )

            bundle(
                "android",
                listOf(
                    "lifecycle-runtime-ktx",
                    "lifecycle-viewmodel-ktx",
                    "lifecycle-viewmodel-savedstate",
                    "paging-runtime-ktx",
                    "room-ktx",
                    "room-paging",
                    "work-runtime-ktx",
                    "core-ktx",
                    "androidx-appcompat",
                    "androidx-preference",
                    "coil.base",
                    "coil.gif",
                    "coil.svg",
                    "kotlin-coroutines-android",
                    "kodein-androidx",
                    "androidx-browser",
                    "emoji2",
                    "emoji2-view-helper",
                ),
            )

            bundle(
                "compose",
                listOf(
                    "activity-compose",
                    "ui",
                    "foundation",
                    "foundation-layout",
                    "compose-material3",
                    "compose-material",
                    "compose-material-icons-extended",
                    "runtime",
                    "ui-tooling",
                    "navigation-compose",
                    "paging-compose",
                    "window",
                    "android-material",
                    "accompanist-permissions",
                    "accompanist-systemuicontroller",
                    "accompanist-adaptive",
                    "compose-material3-windowsizeclass",
                    "lifecycle-runtime-compose",
                    "coil-compose",
                ),
            )

            bundle(
                "test",
                listOf(
                    "kotlin-test-junit",
                    "kotlin-coroutines-test",
                    "junit",
                    "mockito-core",
                    "mockk",
                    "mockwebserver",
                ),
            )

            bundle(
                "android-test",
                listOf(
                    "kotlin-test-junit",
                    "kotlin-coroutines-test",
                    "mockk-android",
                    "junit",
                    "mockwebserver",
                    "androidx-test-core",
                    "androidx-test-core-ktx",
                    "androidx-test-runner",
                    "androidx-test-junit-ktx",
                    "room-testing",
                    "espresso-core",
                    "compose-ui-test-junit4",
                ),
            )
        }
    }
}

rootProject.name = "feeder"

include(":app")
