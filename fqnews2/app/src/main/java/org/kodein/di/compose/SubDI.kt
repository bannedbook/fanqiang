@file:Suppress("ktlint:compose:param-order-check", "ktlint:compose:modifier-composed-check")

package org.kodein.di.compose

import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import org.kodein.di.Copy
import org.kodein.di.DI

/**
 * Allows to create an extended version of a given [DI] container
 * and attaches it to the underlying [Composable] tree
 *
 * @param parentDI the [DI] container that will be copied and extended
 * @param allowSilentOverride Whether the configuration block is allowed to non-explicit overrides.
 * @param copy The copy specifications, that defines which bindings will be copied to the new container.
 *   All bindings from the extended container will be accessible in the new container, but only the copied bindings are able to access overridden bindings in this new container.
 *   By default, all bindings that do not hold references (e.g. not singleton or multiton) are copied.
 * @param diBuilder [DI] container configuration block
 * @param content underlying [Composable] tree that will be able to consume the [DI] container
 */
@Composable
fun SubDI(
    parentDI: DI,
    allowSilentOverride: Boolean = false,
    copy: Copy = Copy.NonCached,
    diBuilder: DI.MainBuilder.() -> Unit,
    content:
        @Composable()
        () -> Unit,
) {
    val di = org.kodein.di.subDI(parentDI, allowSilentOverride, copy) { diBuilder() }
    CompositionLocalProvider(LocalDI provides di) { content() }
}

/**
 * Allows to create an extended version of the [LocalDI] container
 * and attaches it to the underlying [Composable] tree
 *
 * @param allowSilentOverride Whether the configuration block is allowed to non-explicit overrides.
 * @param copy The copy specifications, that defines which bindings will be copied to the new container.
 *   All bindings from the extended container will be accessible in the new container, but only the copied bindings are able to access overridden bindings in this new container.
 *   By default, all bindings that do not hold references (e.g. not singleton or multiton) are copied.
 * @param diBuilder [DI] container configuration block
 * @param content underlying [Composable] tree that will be able to consume the [DI] container
 */
@Composable
fun SubDI(
    allowSilentOverride: Boolean = false,
    copy: Copy = Copy.NonCached,
    diBuilder: DI.MainBuilder.() -> Unit,
    content:
        @Composable()
        () -> Unit,
) = SubDI(LocalDI.current, allowSilentOverride, copy, diBuilder, content)
