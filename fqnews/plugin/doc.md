# Overview

Plugin should be bundled as an apk. `$PLUGIN_ID` in this documentation corresponds to the
 executable name for the plugin in order to be cross-platform, e.g. `obfs-local`. An apk can have
 more than one plugins bundled. We don't care as long as they have different `$PLUGIN_ID`. For
 duplicated plugin ID, host should refuse to start.

There are no arbitrary restrictions/requirements on package name, component name and content
 provider authority, but you're suggested to follow the format in this documentations. For package
 name, use `com.github.shadowsocks.plugin.$PLUGIN_ID` if it only contains a single plugin to prevent
 duplicated plugins. In some places hyphens are not accepted, for example package name. In that
 case, hyphens `-` should be changed into underscores `_`. For example, the package name for
 `obfs-local` would probably be `com.github.shadowsocks.plugin.obfs_local`.

It's advised to use this library for easier development, but you're free to start from scratch following this
 documentation.

# Plugin configuration

Plugins get their args configured via one of the following two options:

* A configuration activity;
  ([example](https://github.com/shadowsocks/simple-obfs-android/tree/4f82c4a4e415d666e70a7e2e60955cb0d85c1615))
* If no configuration activity is found or the activity requests the fallback mode, the fallback
  mode will be used: user manual input and optional help message.
  ([example](https://github.com/shadowsocks/kcptun-android/tree/41f42077e177618553417c16559784a51e9d8c4c))

Your user interface need not be consistent with shadowsocks-android styling - you don't need to use
 preferences UI at all if you don't feel like it - however it's recommended to use Material Design
 at minimum.

## Configuration activity

If the plugin provides a configuration activity, it will be started when user picks your plugin and
 taps configure. It:

* MUST have action: `com.github.shadowsocks.plugin.ACTION_CONFIGURE`;
* MUST have category: `android.intent.category.DEFAULT`;
* MUST be able to receive data URI `plugin://com.github.shadowsocks/$PLUGIN_ID`;
* SHOULD parse string extra `com.github.shadowsocks.plugin.EXTRA_OPTIONS` (all options as a single
  string) and display the current options;
* SHOULD distinguish between server settings and feature settings in some way, e.g. for
  `obfs-local`, `obfs` is a server setting and `obfs_host` is a feature setting;
* On finish, it SHOULD return one of the following results:
  - `RESULT_OK = 0`: In this case it MUST return the data Intent with the new
    `com.github.shadowsocks.plugin.EXTRA_OPTIONS`;
  - `RESULT_CANCELED = -1`: Nothing will be changed;
  - `RESULT_FALLBACK = 1`: Fallback mode is requested and the host should display the fallback
    editor.

This corresponds to `com.github.shadowsocks.plugin.ConfigurationActivity` in the plugin library.
 Here's what a proper configuration activity usually should look like in `AndroidManifest.xml`:

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

## Help activity/callback

If the plugin doesn't provide a configuration activity, it's highly recommended to provide a help
 message in the form of an Activity. It:

* MUST have action: `com.github.shadowsocks.plugin.ACTION_HELP`;
* MUST have category: `android.intent.category.DEFAULT`;
* MUST be able to receive data URI `plugin://com.github.shadowsocks/$PLUGIN_ID`;
* CAN parse string extra `com.github.shadowsocks.plugin.EXTRA_OPTIONS` and display some more
  relevant information;
* SHOULD parse `@NightMode` int extra `com.github.shadowsocks.plugin.EXTRA_NIGHT_MODE` and act
  accordingly;
* SHOULD either:
  - Be invisible and return help message with CharSequence extra
    `com.github.shadowsocks.plugin.EXTRA_HELP_MESSAGE` in the data intent with `RESULT_OK`; (in this
    case, a simple dialog will be shown containing the message)
  - Be visible and return `RESULT_CANCELED`.
* SHOULD distinguish between server settings and feature settings in some way, e.g. for
  `simple_obfs`, `obfs` is a server setting and `obfs_host` is a feature setting.

This corresponds to `com.github.shadowsocks.plugin.HelpActivity` or
 `com.github.shadowsocks.plugin.HelpCallback` in the plugin library. Here's what a proper help
 activity/callback usually should look like in `AndroidManifest.xml`:

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

# Plugin implementation

Every plugin can be either in native mode or JVM mode.

## Native mode

In native mode, plugins are provided as native executables and `shadowsocks-libev`'s plugin mode
 will be used.

Every native mode plugin MUST have a content provider to provide the native executables (since they
 can exceed 1M which is the limit of Intent size) that:

* MUST have `android:label` and `android:icon`; (may be inherited from parent `application`)
* SHOULD have `android:directBootAware="true"` with proper support if possible;
* MUST have an intent filter with action `com.github.shadowsocks.plugin.ACTION_NATIVE_PLUGIN`;
  (used for discovering plugins)
* MUST have meta-data `com.github.shadowsocks.plugin.id` with string value `$PLUGIN_ID` or a string resource;
* MUST have an intent filter with action `com.github.shadowsocks.plugin.ACTION_NATIVE_PLUGIN` and
  data `plugin://com.github.shadowsocks/$PLUGIN_ID`; (used for configuring plugin)
* CAN have meta-data `com.github.shadowsocks.plugin.default_config` with string value or a string resource, default is empty;
* MUST implement `query` that returns the file list which MUST include `$PLUGIN_ID` when having
  these as arguments:
  - `uri = "content://$authority_of_your_provider`;
  - `projection = ["path", "mode"]`; (relative path, for example `obfs-local`; file mode as integer, for
    example `0b110100100`)
  - `selection = null`;
  - `selectionArgs = null`;
  - `sortOrder = null`;
* MUST implement `openFile` that for files returned in `query`, `openFile` with `mode = "r"` returns
  a valid `ParcelFileDescriptor` for reading. For example, `uri` can be
  `content://com.github.shadowsocks.plugin.kcptun/kcptun`.

This corresponds to `com.github.shadowsocks.plugin.NativePluginProvider` in the plugin library.
 Here's what a proper native plugin provider usually should look like in `AndroidManifest.xml`:

```xml
<manifest>
    ...
    <application>
        ...
        <provider android:name=".BinaryProvider"
                  android:exported="true"
                  android:directBootAware="true"
                  android:authorities="$FULLY_QUALIFIED_NAME_OF_YOUR_CONTENTPROVIDER"
                  tools:ignore="ExportedContentProvider">
            <intent-filter>
                <action android:name="com.github.shadowsocks.plugin.ACTION_NATIVE_PLUGIN"/>
            </intent-filter>
            <intent-filter>
                <action android:name="com.github.shadowsocks.plugin.ACTION_NATIVE_PLUGIN"/>
                <data android:scheme="plugin"
                      android:host="com.github.shadowsocks"
                      android:path="/$PLUGIN_ID"/>
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

## Native mode without binary copying

If your plugin binary executable can run in place, you can support native mode without binary
 copying. To support this mode, your `ContentProvider` must first support native mode with binary
 copying (this will be used if the fast routine fails) and:

* MUST implement `call` that returns absolute path to the entry executable as
  `com.github.shadowsocks.plugin.EXTRA_ENTRY` when having `method = "shadowsocks:getExecutable"`;
  (`com.github.shadowsocks.plugin.EXTRA_OPTIONS` is provided in extras as well just in case you
  need them)
* SHOULD define `android:installLocation="internalOnly"` for `<manifest>` in AndroidManifest.xml;
* SHOULD define `android:extractNativeLibs="true"` for `<application>` in AndroidManifest.xml;

If you don't plan to support this mode, you can just throw `UnsupportedOperationException` when
 being invoked. It will fallback to the slow routine automatically.

### Native mode without binary copying and setup

Additionally, if your plugin only needs to supply the path of your executable without doing any extra setup work,
 you can use an additional `meta-data` with name `com.github.shadowsocks.plugin.executable_path`
 to supply executable path to your native binary.
This allows the host app to launch your plugin without ever launching your app.

## JVM mode

This feature hasn't been implemented yet.
Please open an issue if you need this.

# Plugin security

Plugins are certified using package signatures and shadowsocks-android will consider these
 signatures as trusted:

* Signatures by [trusted sources](/mobile/src/main/java/com/github/shadowsocks/plugin/PluginManager.kt#L39)
  which includes:
  - @madeye, i.e. the signer of the main repo;
  - The main repo doesn't contain any other trusted signatures. Third-party forks should add their
    signatures to this trusted sources if they have plugins signed by them before publishing their
    source code.
* Current package signature, which means:
  - If you get apk from shadowsocks-android releases or Google Play, this means only apk signed by
    @madeye will be recognized as trusted.
  - If you get apk from a third-party fork, all plugins from that developer will get recognized as
    trusted automatically even if its source code isn't available anywhere online.

A warning will be shown for untrusted plugins. No arbitrary restrictions will be applied.

# Plugin platform versioning

In order to be able to identify compatible and incompatible plugins, [Semantic
 Versioning](http://semver.org/) will be used.

>Given a version number MAJOR.MINOR.PATCH, increment the:
>
>1. MAJOR version when you make incompatible API changes,
>2. MINOR version when you add functionality in a backwards-compatible manner, and
>3. PATCH version when you make backwards-compatible bug fixes.

Plugin app must include this in their application tag: (which should be automatically included if
 you are using our library)

```
<meta-data android:name="com.github.shadowsocks.plugin.version"
           android:value="1.0.0"/>
```

# Plugin ID Aliasing

To implement plugin ID aliasing, you:

* MUST define meta-data `com.github.shadowsocks.plugin.id.aliases` in your plugin content provider with `android:value="alias"`,
  or use `android:resources` to specify a string resource or string array resource for multiple aliases.
* MUST be able to be matched by `com.github.shadowsocks.plugin.ACTION_NATIVE_PLUGIN` when invoked on alias.
  To do this, you SHOULD use multiple `intent-filter` and use a different `android:path` for each alias.
  Alternatively, you MAY also use a single `intent-filter` and use `android:pathPattern` to match all your aliases at once.
  You MUST NOT use `android:pathPrefix` or allow `android:pathPattern` to match undeclared plugin ID/alias as it might create a conflict with other plugins.
* SHOULD NOT add or change `intent-filter` for activities to include your aliases -- your plugin ID will always be used.

For example:
```xml
<manifest>
    ...
    <application>
        ...
        <provider>
            ...
            <intent-filter>
                <action android:name="com.github.shadowsocks.plugin.ACTION_NATIVE_PLUGIN"/>
                <data android:scheme="plugin"
                      android:host="com.github.shadowsocks"
                      android:path="/$PLUGIN_ID"/>
            </intent-filter>
            <intent-filter>
                <action android:name="com.github.shadowsocks.plugin.ACTION_NATIVE_PLUGIN"/>
                <data android:scheme="plugin"
                      android:host="com.github.shadowsocks"
                      android:path="/$PLUGIN_ALIAS"/>
            </intent-filter>
            <meta-data android:name="com.github.shadowsocks.plugin.id"
                       android:value="$PLUGIN_ID"/>
            <meta-data android:name="com.github.shadowsocks.plugin.aliases"
                       android:value="$PLUGIN_ALIAS"/>
            ...
        </provider>
        ...
    </application>
</manifest>
```

# Android TV

Android TV client does not invoke configuration activities. Therefore your plugins should automatically work with them.
