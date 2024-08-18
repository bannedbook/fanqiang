package org.kodein.di.compose

import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import org.kodein.di.DIProperty
import org.kodein.di.factory
import org.kodein.di.instance
import org.kodein.di.provider

/**
 * Gets an instance of `T` for the given type and tag.
 *
 * T generics will be preserved!
 *
 * @param T The type of object to retrieve.
 * @param tag The bound tag, if any.
 * @return An instance.
 * @throws DI.NotFoundException if no provider was found.
 * @throws DI.DependencyLoopException If the instance construction triggered a dependency loop.
 */
@Composable
inline fun <reified T : Any> instance(tag: Any? = null): DIProperty<T> =
    with(LocalDI.current) {
        remember { instance(tag = tag) }
    }

/**
 * Gets an instance of [T] for the given type and tag, curried from a factory that takes an argument [A].
 *
 * A & T generics will be preserved!
 *
 * @param A The type of argument the curried factory takes.
 * @param T The type of object to retrieve.
 * @param tag The bound tag, if any.
 * @param arg The argument that will be given to the factory when curried.
 * @return An instance of [T].
 * @throws DI.NotFoundException If no provider was found.
 * @throws DI.DependencyLoopException If the value construction triggered a dependency loop.
 */
@Composable
inline fun <reified A : Any, reified T : Any> instance(
    arg: A,
    tag: Any? = null,
): DIProperty<T> =
    with(LocalDI.current) {
        remember { instance(tag = tag, arg = arg) }
    }

/**
 * Gets a factory of `T` for the given argument type, return type and tag.
 *
 * A & T generics will be preserved!
 *
 * @param A The type of argument the factory takes.
 * @param T The type of object the factory returns.
 * @param tag The bound tag, if any.
 * @return A factory.
 * @throws DI.NotFoundException if no factory was found.
 * @throws DI.DependencyLoopException When calling the factory function, if the instance construction triggered a dependency loop.
 */
@Composable
inline fun <reified A : Any, reified T : Any> factory(tag: Any? = null): DIProperty<(A) -> T> =
    with(LocalDI.current) {
        remember { factory(tag = tag) }
    }

/**
 * Gets a provider of `T` for the given type and tag.
 *
 * T generics will be preserved!
 *
 * @param T The type of object the provider returns.
 * @param tag The bound tag, if any.
 * @return A provider.
 * @throws DI.NotFoundException if no provider was found.
 * @throws DI.DependencyLoopException When calling the provider function, if the instance construction triggered a dependency loop.
 */
@Composable
inline fun <reified A : Any, reified T : Any> provider(tag: Any? = null): DIProperty<() -> T> =
    with(LocalDI.current) {
        remember { provider(tag = tag) }
    }

/**
 * Gets a provider of [T] for the given type and tag, curried from a factory that takes an argument [A].
 *
 * A & T generics will be preserved!
 *
 * @param A The type of argument the curried factory takes.
 * @param T The type of object to retrieve with the returned provider.
 * @param tag The bound tag, if any.
 * @param arg The argument that will be given to the factory when curried.
 * @return A provider of [T].
 * @throws DI.NotFoundException If no provider was found.
 * @throws DI.DependencyLoopException When calling the provider, if the value construction triggered a dependency loop.
 */
@Composable
inline fun <reified A : Any, reified T : Any> provider(
    arg: A,
    tag: Any? = null,
): DIProperty<() -> T> =
    with(LocalDI.current) {
        remember { provider(tag = tag, arg = arg) }
    }

/**
 * Gets a provider of [T] for the given type and tag, curried from a factory that takes an argument [A].
 *
 * A & T generics will be preserved!
 *
 * @param A The type of argument the curried factory takes.
 * @param T The type of object to retrieve with the returned provider.
 * @param tag The bound tag, if any.
 * @param fArg A function that returns the argument that will be given to the factory when curried.
 * @return A provider of [T].
 * @throws DI.NotFoundException If no provider was found.
 * @throws DI.DependencyLoopException When calling the provider, if the value construction triggered a dependency loop.
 */
@Composable
inline fun <reified A : Any, reified T : Any> provider(
    noinline fArg: () -> A,
    tag: Any? = null,
): DIProperty<() -> T> =
    with(LocalDI.current) {
        remember { provider(tag = tag, arg = fArg) }
    }
