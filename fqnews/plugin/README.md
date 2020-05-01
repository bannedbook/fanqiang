# shadowsocks-android plugin framework

[Documentation](doc.md) | [Change log](CHANGES.md)

Support library for easier development on [shadowsocks
 plugin](https://github.com/shadowsocks/shadowsocks-org/issues/28) for Android. Also includes some
 useful resources to easily get consistent styling with the main app.

## Official plugins

These are some plugins ready to use on shadowsocks-android.

* [simple-obfs](https://github.com/shadowsocks/simple-obfs-android/releases)
* [kcptun](https://github.com/shadowsocks/kcptun-android/releases)

## Developer's guide

WARNING: This library is still in beta (0.x) and its content is subject to massive changes.

This library is designed with Java interoperability in mind so theoretically you can use this
 library with other languages and/or build tools but there isn't documentation for that yet. This
 guide is written for Scala + SBT. Contributions are welcome.

### Package name

There are no arbitrary restrictions/requirements on package name, component name and content
 provider authority, but you're suggested to follow the format in this documentations. For package
 name, use `com.github.shadowsocks.plugin.$PLUGIN_ID` if it only contains a single plugin to
 prevent duplicated plugins. In some places hyphens are not accepted, for example package name. In
 that case, hyphens `-` should be changed into underscores `_`. For example, the package name for
 `obfs-local` would probably be `com.github.shadowsocks.plugin.obfs_local`.

### Add dependency

First you need to add this library to your dependencies. This library is written mostly in Scala
 and it's most convenient to use it with SBT:

```scala
libraryDependencies += "com.github.shadowsocks" %% "plugin" % "0.0.4"
```

### Native binary configuration

First you need to get your native binary compiling on Android platform.

* [Sample project for C](https://github.com/shadowsocks/simple-obfs-android/tree/4f82c4a4e415d666e70a7e2e60955cb0d85c1615);
* [Sample project for Go](https://github.com/shadowsocks/kcptun-android/tree/41f42077e177618553417c16559784a51e9d8c4c).

In addition to functionalities of a normal plugin, it has to support these additional flags that
 may get passed through arguments:

* `-V`: VPN mode. In this case, the plugin should pass all file descriptors that needs protecting
  from VPN connections (i.e. its traffic will not be forwarded through the VPN) through an
  ancillary message to `./protect_path`;
* `--fast-open`: TCP fast open enabled.

### Implement a binary provider

It's super easy. You just need to implement two or three methods. For example for `obfs-local`:

```scala
final class BinaryProvider extends NativePluginProvider {
  override protected def populateFiles(provider: PathProvider) {
    provider.addPath("obfs-local", "755")
    // add additional files here
  }

  // remove this method to disable fast mode, read more in the documentation
  override def getExecutable: String =
    getContext.getApplicationInfo.nativeLibraryDir + "/libobfs-local.so"

  override def openFile(uri: Uri): ParcelFileDescriptor = uri.getPath match {
    case "/obfs-local" =>
      ParcelFileDescriptor.open(new File(getExecutable), ParcelFileDescriptor.MODE_READ_ONLY)
    // handle additional files here
    case _ => throw new FileNotFoundException()
  }
}
```

Then add it to your manifest:

```xml
<manifest>
    ...
    <application>
        ...
        <provider android:name=".BinaryProvider"
                  android:exported="true"
                  android:authorities="$FULLY_QUALIFIED_NAME_OF_YOUR_CONTENTPROVIDER">
            <intent-filter>
                <action android:name="com.github.shadowsocks.plugin.ACTION_NATIVE_PLUGIN"/>
            </intent-filter>
            <intent-filter>
                <action android:name="com.github.shadowsocks.plugin.ACTION_NATIVE_PLUGIN"/>
                <data android:scheme="plugin"
                      android:host="com.github.shadowsocks"
                      android:pathPrefix="/$PLUGIN_ID"/>
            </intent-filter>
            <meta-data android:name="com.github.shadowsocks.plugin.id"
                       android:value="$PLUGIN_ID"/>
            <meta-data android:name="com.github.shadowsocks.plugin.default_config"
                       android:value="dummy=default;plugin=options"/>
        </provider>
        ...
    </application>
</manifest>
```

### Add user interfaces

You should add to your plugin app a configuration activity or a help activity or both if you're
 going to use `ConfigurationActivity.fallbackToManualEditor`.

#### Configuration activity

This is used if found instead of a manual input dialog when user clicks "Configure..." in the main
 app. This gives you maximum freedom of the user interface. To implement this, you need to extend
 `ConfigurationActivity` and you will get current options via
 `onInitializePluginOptions(PluginOptions)` and you can invoke `saveChanges(PluginOptions)` or
 `discardChanges()` before `finish()` or `fallbackToManualEditor()`. Then add it to your manifest:

```xml
<manifest>
    ...
    <application>
        ...
        <activity android:name=".ConfigActivity">
            <intent-filter>
                <action android:name="com.github.shadowsocks.plugin.ACTION_CONFIGURE"/>
                <category android:name="android.intent.category.DEFAULT"/>
                <data android:scheme="plugin"
                      android:host="com.github.shadowsocks"
                      android:path="/$PLUGIN_ID"/>
            </intent-filter>
        </activity>
        ...
    </application>
</manifest>
```

#### Help activity/callback

This is started when user taps "?" in manual editor. To implement this, you need to extend
 `HelpCallback` if you want a simple dialog with help message as `CharSequence` or `HelpActivity`
 if you want to provide custom user interface, implement the required methods, then add it to your
 manifest:

```xml
<manifest>
    ...
    <application>
        ...
        <activity android:name=".HelpActivity">
            <intent-filter>
                <action android:name="com.github.shadowsocks.plugin.ACTION_HELP"/>
                <category android:name="android.intent.category.DEFAULT"/>
                <data android:scheme="plugin"
                      android:host="com.github.shadowsocks"
                      android:path="/$PLUGIN_ID"/>
            </intent-filter>
        </activity>
        ...
    </application>
</manifest>
```

Great. Now your plugin is ready to use.
