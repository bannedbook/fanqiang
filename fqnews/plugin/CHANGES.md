* 1.3.4:
  * Optional new metadata `com.github.shadowsocks.plugin.id.aliases` for plugin ID aliases;
    (see doc for `PluginContract.METADATA_KEY_ID_ALIASES` and main documentation "Plugin ID Aliasing" for more information)
  * Please use `android:path` instead of `android:pathPrefix`, sample code in documentations have been updated to reflect this recommendation.
  * Added missing documentation regarding direct boot support.
    Please add `android:directBootAware="true"` with proper support for your `provider` if possible.
  * You can now use `android:resources` on `meta-data` tags. (main/host app update required, however, you should never use dynamic resources)
  * Fix occasional crash in `AlertDialogFragment`.
  * Translation updates.
  * Dependency updates:
    - `androidx.core:core-ktx:1.2.0`;
    - `com.google.android.material:material:1.1.0`.
* 1.3.3:
  * Fix a build script issue.
* 1.3.2:
  * Fix first key-value pair disappearing with null value. (#2391)
  * Dependency updates:
    - `androidx.core:core-ktx:1.1.0`;
    - `com.google.android.material:material:1.1.0-rc01`;
    - `org.jetbrains.kotlin:kotlin-stdlib-jdk8:1.3.61`.
* 1.3.1:
  * New theme resource `Theme.Shadowsocks.Immersive` for better Android Q-esque translucent navigation bars.
    This is an opt-in feature.
    Please add `android:theme="@style/Theme.Shadowsocks.Immersive"` to your `<activity>` to enable this theme.
  * New color resources `light_*` and `dark_*` for passing to custom tabs;
  * Dependency updates:
    - `androidx.core:core-ktx:1.1.0-rc03`;
    - `androidx.drawerlayout:drawerlayout:1.1.0-alpha03`;
    - `com.google.android.material:material:1.1.0-alpha09`;
    - `org.jetbrains.kotlin:kotlin-stdlib-jdk8:1.3.41`.
* 1.3.0:
  * Optional new metadata `com.github.shadowsocks.plugin.executable_path` for even faster initialization;
    (see doc for `PluginContract.METADATA_KEY_EXECUTABLE_PATH` for more information)
  * Breaking API change: `val AlertDialogFragment.ret: Ret?` => `fun AlertDialogFragment.ret(which: Int): Ret?`;
    (nothing needs to be done if you are not using this API)
  * Dependency updates:
    - Now targeting API 29;
    - `androidx.core:core-ktx:1.1.0-rc01`;
    - `com.google.android.material:material:1.1.0-alpha07`;
    - `org.jetbrains.kotlin:kotlin-stdlib-jdk8:1.3.40`.
* 1.2.0:
  * New helper class `AlertDialogFragment` for creating `AlertDialog` that persists through configuration changes;
  * Dependency update: `com.google.android.material:material:1.1.0-alpha03`.
* 1.1.0:
  * Having control characters in plugin options is no longer allowed.
    If this breaks your plugin, you are doing it wrong.
  * New helper method: `PluginOptions.putWithDefault`.
* 1.0.0:
  * BREAKING CHANGE: Plugins developed using this version and forward require shadowsocks-android 4.6.5 or higher.
  * `PathProvider` now takes `Int` instead of `String` for file modes;
  * Refactor to AndroidX;
  * No longer depends on preference libraries.
* 0.1.1:
  * Rewritten in Kotlin;
  * Fix assert not working;
  * Min API 21;
  * Update support library version to 27.1.1.
* 0.0.4:
  * Enlarge text size of number pickers;
  * Update support library version to 26.0.0.
* 0.0.3:
  * Update support library version to 25.2.0.
* 0.0.2:
  * Add `getOrDefault` to `PluginOptions`;
  * Update support library version to 25.1.1.
* 0.0.1: Initial release.
