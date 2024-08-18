import java.util.Locale

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
    alias(libs.plugins.kotlin.ksp)
    alias(libs.plugins.kotlin.parcelize)
    alias(libs.plugins.kotlin.serialization)
    alias(libs.plugins.ktlint.gradle)
}

android {
    namespace = "com.nononsenseapps.feeder"
    compileSdk = 34

    defaultConfig {
        applicationId = "jww.feed.fqnews"
        versionCode = 292
        versionName = "1.3.8"
        minSdk = 23
        targetSdk = 34

        vectorDrawables.useSupportLibrary = true

        resourceConfigurations.addAll(getListOfSupportedLocales())

        // For espresso tests
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    flavorDimensions += "site"
    productFlavors {
        create("fqnews") {
            dimension  = "site"
            applicationId = "jww.feed.fqnews"
            buildConfigField("String", "applicationId", "\""+applicationId!!+"\"")
            buildConfigField("String", "admob_appid", "\"ca-app-pub-2194043486084479~7068327579\"")
            buildConfigField("String", "banner_adUnitId", "\"ca-app-pub-2194043486084479/5355559146\"")
            buildConfigField("String", "native_adUnitId", "\"ca-app-pub-2194043486084479/2721000405\"")
            buildConfigField("String", "interstitial_adUnitId", "\"ca-app-pub-2194043486084479/2288264870\"")
        }


    }
    sourceSets {
        getByName("fqnews").setRoot("src/fqnews")
    }
    androidComponents {
        onVariants { variant ->
            val taskName = "print${variant.name.replaceFirstChar { if (it.isLowerCase()) it.titlecase(Locale.getDefault()) else it.toString() }}ApplicationId"
            tasks.register(taskName) {
                doLast {
                    println("Application ID for ${variant.name} is ${variant.applicationId.get()}")
                }
            }
        }
    }

    ksp {
        arg(RoomSchemaArgProvider(File(projectDir, "schemas")))
    }

    sourceSets {
        // To test Room we need to include the schema dir in resources
        named("androidTest") {
            assets.srcDir("$projectDir/schemas")
        }
    }

    signingConfigs {
        create("shareddebug") {
            storeFile = rootProject.file("shareddebug.keystore")
            storePassword = "android"
            keyAlias = "AndroidDebugKey"
            keyPassword = "android"
        }
        if (project.hasProperty("STORE_FILE")) {
            create("release") {
                @Suppress("LocalVariableName", "ktlint:standard:property-naming")
                val STORE_FILE: String by project.properties

                @Suppress("LocalVariableName", "ktlint:standard:property-naming")
                val STORE_PASSWORD: String by project.properties

                @Suppress("LocalVariableName", "ktlint:standard:property-naming")
                val KEY_ALIAS: String by project.properties

                @Suppress("LocalVariableName", "ktlint:standard:property-naming")
                val KEY_PASSWORD: String by project.properties
                storeFile = file(STORE_FILE)
                storePassword = STORE_PASSWORD
                keyAlias = KEY_ALIAS
                keyPassword = KEY_PASSWORD
            }
        }
    }

    buildTypes {
        val debug by getting {
            isMinifyEnabled = false
            isShrinkResources = false
            applicationIdSuffix = ".debug"
            isPseudoLocalesEnabled = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro",
            )
            signingConfig = signingConfigs.getByName("shareddebug")
        }
        val release by getting {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro",
            )
            if (project.hasProperty("STORE_FILE")) {
                signingConfig = signingConfigs.getByName("release")
            }
//            else {
//                signingConfig = signingConfigs.getByName("shareddebug")
//            }
        }
        val play by creating {
            //applicationIdSuffix = ".play"
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro",
            )
            if (project.hasProperty("STORE_FILE")) {
                signingConfig = signingConfigs.getByName("release")
            }
//            else {
//                signingConfig = signingConfigs.getByName("shareddebug")
//            }
        }
    }

    testOptions {
        unitTests {
            isReturnDefaultValues = true
        }
        managedDevices {
            devices {
                maybeCreate<com.android.build.api.dsl.ManagedVirtualDevice>("pixel2api30").apply {
                    // Use device profiles you typically see in Android Studio.
                    device = "Pixel 2"
                    // Use only API levels 27 and higher.
                    apiLevel = 30
                    // To include Google services, use "google".
                    systemImageSource = "aosp-atd"
                }
            }
        }
    }

    kotlinOptions {
        jvmTarget = "1.8"
    }

    compileOptions {
        isCoreLibraryDesugaringEnabled = true
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }

    buildFeatures {
        compose = true
        buildConfig = true
        aidl = true
        renderScript = false
        resValues = false
        shaders = false
    }

    composeOptions {
        kotlinCompilerExtensionVersion = libs.versions.composeCompiler.get()
    }
    packaging {
        resources {
            excludes.addAll(
                listOf(
                    "META-INF/DEPENDENCIES",
                    "META-INF/LICENSE",
                    "META-INF/LICENSE.md",
                    "META-INF/LICENSE.txt",
                    "META-INF/license.txt",
                    "META-INF/LICENSE-notice.md",
                    "META-INF/NOTICE",
                    "META-INF/NOTICE.txt",
                    "META-INF/notice.txt",
                    "META-INF/ASL2.0",
                    "META-INF/AL2.0",
                    "META-INF/LGPL2.1",
                ),
            )
        }
    }

    lint {
        abortOnError = true
        disable.addAll(listOf("MissingTranslation", "AppCompatCustomView", "InvalidPackage"))
        error.addAll(listOf("InlinedApi", "StringEscaping"))
        explainIssues = true
        ignoreWarnings = true
        textOutput = file("stdout")
        textReport = true
    }
}

// gw installDebug -Pmyapp.enableComposeCompilerReports=true --rerun-tasks
tasks.withType(org.jetbrains.kotlin.gradle.tasks.KotlinCompile::class.java) {
    kotlinOptions {
        if (project.findProperty("myapp.enableComposeCompilerReports") == "true") {
            freeCompilerArgs +=
                listOf(
                    "-P",
                    "plugin:androidx.compose.compiler.plugins.kotlin:reportsDestination=${project.buildDir.resolve("compose_metrics").canonicalPath}",
                )
            freeCompilerArgs +=
                listOf(
                    "-P",
                    "plugin:androidx.compose.compiler.plugins.kotlin:metricsDestination=${project.buildDir.resolve("compose_metrics").canonicalPath}",
                )
        }
    }
}

configurations.all {
    resolutionStrategy {
//    failOnVersionConflict()
    }
}

dependencies {
    ktlintRuleset(libs.ktlint.compose)
    ksp(libs.room)
    // For java time
    coreLibraryDesugaring(libs.desugar)

    // BOMS
    implementation(platform(libs.okhttp.bom))
    implementation(platform(libs.coil.bom))
    implementation(platform(libs.compose.bom))

    // Dependencies
    implementation(libs.bundles.android)
    implementation(libs.bundles.compose)
    implementation(libs.bundles.jvm)
    implementation(libs.bundles.okhttp.android)
    implementation(libs.bundles.kotlin)

    // Only for debug
    debugImplementation("com.squareup.leakcanary:leakcanary-android:3.0-alpha-1")

    // Tests
    testImplementation(libs.bundles.kotlin)
    testImplementation(libs.bundles.test)

    androidTestImplementation(platform(libs.compose.bom))
    androidTestImplementation(libs.bundles.kotlin)
    androidTestImplementation(libs.bundles.android.test)

    debugImplementation(libs.compose.ui.test.manifest)

    //for fqnews only
    implementation(files("../libcore/libcore.aar")) // 使用 files() 方法加载 .aar 文件
    implementation("com.github.MatrixDev.Roomigrant:RoomigrantLib:0.3.4")
    //kapt("com.github.MatrixDev.Roomigrant:RoomigrantCompiler:0.3.4")
    implementation("com.esotericsoftware:kryo:5.2.1")
    implementation("com.jakewharton:process-phoenix:2.1.2")
    implementation("com.google.code.gson:gson:2.9.0")
    implementation("androidx.room:room-runtime:2.5.1")
    //kapt("androidx.room:room-compiler:2.5.1")
    implementation("androidx.room:room-ktx:2.5.1")
    implementation("com.github.MatrixDev.Roomigrant:RoomigrantLib:0.3.4")
    //kapt("com.github.MatrixDev.Roomigrant:RoomigrantCompiler:0.3.4")
    implementation("org.ini4j:ini4j:0.5.4")
    implementation("org.yaml:snakeyaml:1.30")
    //implementation ("com.google.android.material:material:1.4.0")  //Snackbar
}

fun getListOfSupportedLocales(): List<String> {
    val resFolder = file(projectDir.resolve("src/main/res"))

    return resFolder.list { _, s ->
        s.startsWith("values")
    }?.filter { folder ->
        val stringsSize = resFolder.resolve("$folder/strings.xml").length()
        // values/strings.xml is over 13k in size so this filters out too partial translations
        stringsSize > 10_000L
    }?.map { folder ->
        if (folder == "values") {
            "en"
        } else {
            folder.substringAfter("values-")
        }
    }?.sorted()
        ?: listOf("en")
}

tasks {
    register("generateLocalesConfig") {
        val resFolder = file(projectDir.resolve("src/main/res"))
        inputs.files(
            resFolder.listFiles { file ->
                file.name.startsWith("values")
            }?.map { file ->
                file.resolve("strings.xml")
            } ?: error("Could not resolve values folders!"),
        )

        val localesConfigFile = file(projectDir.resolve("src/main/res/xml/locales_config.xml"))
        outputs.file(projectDir.resolve("src/main/res/xml/locales_config.xml"))

        doLast {
            val langs = getListOfSupportedLocales()
            val localesConfig =
                """
                <?xml version="1.0" encoding="utf-8"?>
                <locale-config xmlns:android="http://schemas.android.com/apk/res/android">
                ${langs.joinToString(" ") { "<locale android:name=\"$it\"/>" }}
                </locale-config>
                """.trimIndent()

            localesConfigFile.bufferedWriter().use { writer ->
                writer.write(localesConfig)
            }
        }
    }
}

class RoomSchemaArgProvider(
    @get:InputDirectory
    @get:PathSensitive(PathSensitivity.RELATIVE)
    val schemaDir: File,
) : CommandLineArgumentProvider {
    override fun asArguments(): Iterable<String> {
        // Note: If you're using KSP, change the line below to return
        return listOf("room.schemaLocation=${schemaDir.path}")
    }
}
