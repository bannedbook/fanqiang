import org.jetbrains.kotlin.gradle.tasks.KotlinCompile

buildscript {
    repositories {
        mavenCentral()
        google()
    }
}

plugins {
    alias(libs.plugins.android.application).apply(false)
    alias(libs.plugins.kotlin.android).apply(false)
    alias(libs.plugins.kotlin.ksp).apply(false)
    alias(libs.plugins.kotlin.parcelize).apply(false)
    alias(libs.plugins.kotlin.jvm).apply(false)
    alias(libs.plugins.kotlin.serialization).apply(false)
    alias(libs.plugins.ktlint.gradle).apply(false)
}

allprojects {
    tasks.withType<JavaCompile>().configureEach {
        options.encoding = "UTF-8"
    }
}

subprojects {
    tasks.withType<KotlinCompile>().configureEach {
        kotlinOptions {
            // Treat all Kotlin warnings as errors
            allWarningsAsErrors = false

//      freeCompilerArgs += '-Xopt-in=kotlin.RequiresOptIn'
//      freeCompilerArgs += '-Xopt-in=kotlin.Experimental'

            // Set JVM target to 1.8
            jvmTarget = "1.8"
        }
    }
}
