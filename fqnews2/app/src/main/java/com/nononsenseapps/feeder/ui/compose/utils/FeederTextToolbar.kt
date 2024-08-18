package com.nononsenseapps.feeder.ui.compose.utils

import android.content.ClipboardManager
import android.content.ComponentName
import android.content.Context
import android.content.Context.CLIPBOARD_SERVICE
import android.content.Intent
import android.content.pm.PackageManager
import android.content.pm.ResolveInfo
import android.os.Build
import android.util.Log
import android.view.ActionMode
import android.view.Menu
import android.view.MenuItem
import android.view.View
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.platform.LocalTextToolbar
import androidx.compose.ui.platform.LocalView
import androidx.compose.ui.platform.TextToolbar
import androidx.compose.ui.platform.TextToolbarStatus
import com.nononsenseapps.feeder.util.ActivityLauncher
import org.kodein.di.compose.LocalDI
import org.kodein.di.instance

private const val LOG_TAG = "FEEDER_TEXTTOOL"

@Composable
fun WithFeederTextToolbar(content: @Composable () -> Unit) {
    val activityLauncher: ActivityLauncher by LocalDI.current.instance()
    CompositionLocalProvider(LocalTextToolbar provides FeederTextToolbar(LocalView.current, activityLauncher)) {
        content()
    }
}

class FeederTextToolbar(private val view: View, activityLauncher: ActivityLauncher) : TextToolbar {
    private var actionMode: ActionMode? = null
    private val textActionModeCallback: FeederTextActionModeCallback =
        FeederTextActionModeCallback(
            context = view.context,
            activityLauncher = activityLauncher,
            onActionModeDestroy = {
                actionMode = null
            },
        )
    override var status: TextToolbarStatus = TextToolbarStatus.Hidden
        private set

    override fun hide() {
        status = TextToolbarStatus.Hidden
        actionMode?.finish()
        actionMode = null
    }

    override fun showMenu(
        rect: Rect,
        onCopyRequested: (() -> Unit)?,
        onPasteRequested: (() -> Unit)?,
        onCutRequested: (() -> Unit)?,
        onSelectAllRequested: (() -> Unit)?,
    ) {
        textActionModeCallback.rect = rect
        textActionModeCallback.onCopyRequested = onCopyRequested
        textActionModeCallback.onCutRequested = onCutRequested
        textActionModeCallback.onPasteRequested = onPasteRequested
        textActionModeCallback.onSelectAllRequested = onSelectAllRequested
        if (actionMode == null) {
            status = TextToolbarStatus.Shown
            actionMode =
                view.startActionMode(
                    FloatingTextActionModeCallback(textActionModeCallback),
                    ActionMode.TYPE_FLOATING,
                )
        } else {
            actionMode?.invalidate()
        }
    }
}

class FeederTextActionModeCallback(
    val context: Context,
    val onActionModeDestroy: (() -> Unit)? = null,
    var rect: Rect = Rect.Zero,
    val activityLauncher: ActivityLauncher,
    var onCopyRequested: (() -> Unit)? = null,
    var onPasteRequested: (() -> Unit)? = null,
    var onCutRequested: (() -> Unit)? = null,
    var onSelectAllRequested: (() -> Unit)? = null,
) : ActionMode.Callback {
    private val displayNameComparator by lazy {
        ResolveInfo.DisplayNameComparator(packageManager)
    }
    private val packageManager by lazy {
        context.packageManager
    }
    private val clipboardManager by lazy {
        context.getSystemService(CLIPBOARD_SERVICE) as ClipboardManager
    }

    private val textProcessors = mutableListOf<ComponentName>()

    override fun onCreateActionMode(
        mode: ActionMode?,
        menu: Menu?,
    ): Boolean {
        requireNotNull(menu)
        requireNotNull(mode)

        onCopyRequested?.let {
            addMenuItem(menu, MenuItemOption.Copy)
        }
        onPasteRequested?.let {
            addMenuItem(menu, MenuItemOption.Paste)
        }
        onCutRequested?.let {
            addMenuItem(menu, MenuItemOption.Cut)
        }
        onSelectAllRequested?.let {
            addMenuItem(menu, MenuItemOption.SelectAll)
        }
        onCopyRequested?.let {
            // Depends on copy/paste
            addTextProcessors(menu)
        }
        return true
    }

    override fun onPrepareActionMode(
        mode: ActionMode?,
        menu: Menu?,
    ): Boolean {
        if (mode == null || menu == null) return false
        updateMenuItems(menu)
        // should return true so that new menu items are populated
        return true
    }

    override fun onActionItemClicked(
        mode: ActionMode?,
        item: MenuItem?,
    ): Boolean {
        when (val itemId = item!!.itemId) {
            MenuItemOption.Copy.id -> onCopyRequested?.invoke()
            MenuItemOption.Paste.id -> onPasteRequested?.invoke()
            MenuItemOption.Cut.id -> onCutRequested?.invoke()
            MenuItemOption.SelectAll.id -> onSelectAllRequested?.invoke()
            else -> {
                if (itemId < 100) return false

                // Since we can't access the selected text - hack it by using the clipboard
                val prevClip = clipboardManager.primaryClip
                onCopyRequested?.invoke()

                val clip = clipboardManager.primaryClip
                if (clip != null && clip.itemCount > 0) {
                    textProcessors.getOrNull(itemId - 100)?.let { cn ->
                        activityLauncher.startActivity(
                            openAdjacentIfSuitable = true,
                            intent =
                                Intent(Intent.ACTION_PROCESS_TEXT).apply {
                                    type = "text/plain"
                                    component = cn
                                    putExtra(Intent.EXTRA_PROCESS_TEXT, clip.getItemAt(0).text)
                                },
                        )
                    }
                }

                try {
                    prevClip?.let { clipboardManager.setPrimaryClip(it) }
                } catch (e: Exception) {
                    // This can crash if the content contains a fileUri for example
                    // android.os.FileUriExposedException: file:/// exposed beyond app through ClipData.Item.getUri()
                    Log.e(LOG_TAG, "Resetting clipboard failed", e)
                }
            }
        }
        mode?.finish()
        return true
    }

    override fun onDestroyActionMode(mode: ActionMode?) {
        onActionModeDestroy?.invoke()
    }

    private fun addTextProcessors(menu: Menu) {
        textProcessors.clear()

        val intent =
            Intent(Intent.ACTION_PROCESS_TEXT).apply {
                type = "text/plain"
            }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            packageManager.queryIntentActivities(intent, PackageManager.ResolveInfoFlags.of(0L))
        } else {
            @Suppress("DEPRECATION")
            packageManager.queryIntentActivities(intent, 0)
        }
            .sortedWith(displayNameComparator)
            .forEachIndexed { index, info ->
                val label = info.loadLabel(packageManager)
                val id = 100 + index
                if (menu.findItem(id) == null) {
                    // groupId, itemId, order, title
                    menu.add(1, id, id, label)
                        .setShowAsAction(MenuItem.SHOW_AS_ACTION_NEVER)
                }

                textProcessors.add(
                    ComponentName(
                        info.activityInfo.applicationInfo.packageName,
                        info.activityInfo.name,
                    ),
                )
            }
    }

    private fun updateMenuItems(menu: Menu) {
        addOrRemoveMenuItem(menu, MenuItemOption.Copy, onCopyRequested)
        addOrRemoveMenuItem(menu, MenuItemOption.Paste, onPasteRequested)
        addOrRemoveMenuItem(menu, MenuItemOption.Cut, onCutRequested)
        addOrRemoveMenuItem(menu, MenuItemOption.SelectAll, onSelectAllRequested)
        onCopyRequested?.let {
            // Depends on copy/paste
            addTextProcessors(menu)
        }
    }

    private fun addMenuItem(
        menu: Menu,
        item: MenuItemOption,
    ) {
        menu.add(0, item.id, item.order, item.titleResource)
            .setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM)
    }

    private fun addOrRemoveMenuItem(
        menu: Menu,
        item: MenuItemOption,
        callback: (() -> Unit)?,
    ) {
        when {
            callback != null && menu.findItem(item.id) == null -> addMenuItem(menu, item)
            callback == null && menu.findItem(item.id) != null -> menu.removeItem(item.id)
        }
    }
}

internal enum class MenuItemOption(val id: Int) {
    Copy(0),
    Paste(1),
    Cut(2),
    SelectAll(3),
    ;

    val titleResource: Int
        get() =
            when (this) {
                Copy -> android.R.string.copy
                Paste -> android.R.string.paste
                Cut -> android.R.string.cut
                SelectAll -> android.R.string.selectAll
            }

    /**
     * This item will be shown before all items that have order greater than this value.
     */
    val order = id
}

internal class FloatingTextActionModeCallback(
    private val callback: FeederTextActionModeCallback,
) : ActionMode.Callback2() {
    override fun onActionItemClicked(
        mode: ActionMode?,
        item: MenuItem?,
    ): Boolean {
        return callback.onActionItemClicked(mode, item)
    }

    override fun onCreateActionMode(
        mode: ActionMode?,
        menu: Menu?,
    ): Boolean {
        return callback.onCreateActionMode(mode, menu)
    }

    override fun onPrepareActionMode(
        mode: ActionMode?,
        menu: Menu?,
    ): Boolean {
        return callback.onPrepareActionMode(mode, menu)
    }

    override fun onDestroyActionMode(mode: ActionMode?) {
        callback.onDestroyActionMode(mode)
    }

    override fun onGetContentRect(
        mode: ActionMode?,
        view: View?,
        outRect: android.graphics.Rect?,
    ) {
        val rect = callback.rect
        outRect?.set(
            rect.left.toInt(),
            rect.top.toInt(),
            rect.right.toInt(),
            rect.bottom.toInt(),
        )
    }
}
