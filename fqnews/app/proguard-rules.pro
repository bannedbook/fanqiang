# Add project specific ProGuard rules here.
# By default, the flags in this file are appended to flags specified
# in /home/fred/Android/Sdk/tools/proguard/proguard-android.txt
# You can edit the include path and order by changing the proguardFiles
# directive in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html


# for OOS, no need to remove all info
-renamesourcefileattribute SourceFile
-keepattributes SourceFile,LineNumberTable
-keepnames class *
-keepclasseswithmembernames class * { *; }

# for the search
-keep class android.support.v7.widget.SearchView { *; }

# for anko
-dontwarn org.jetbrains.anko.internals.AnkoInternals

# for glide
-keep public class * implements com.bumptech.glide.module.GlideModule
-keep public class * extends com.bumptech.glide.AppGlideModule
-keep public enum com.bumptech.glide.load.resource.bitmap.ImageHeaderParser$** {
  **[] $VALUES;
  public *;
}

# OkHttp
-keepattributes Signature
-keepattributes *Annotation*
-keep class okhttp3.** { *; }
-keep interface okhttp3.** { *; }
-dontwarn okhttp3.**
-dontwarn okio.**

# Rome lib
-keep class com.rometools.** { *; }
-dontwarn java.beans.**
-dontwarn javax.**
-dontwarn org.jaxen.**
-dontwarn org.slf4j.**
