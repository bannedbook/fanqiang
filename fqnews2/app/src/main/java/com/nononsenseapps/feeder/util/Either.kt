/**
 * The code in this file was based on Arrow, with the following license
 *
 * Copyright (C) 2017 The Arrow Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
@file:OptIn(ExperimentalContracts::class)
@file:Suppress("NOTHING_TO_INLINE", "unused")

package com.nononsenseapps.feeder.util

import kotlin.contracts.ExperimentalContracts
import kotlin.contracts.InvocationKind
import kotlin.contracts.contract

sealed class Either<out A, out B> {
    @OptIn(ExperimentalContracts::class)
    fun isLeft(): Boolean {
        contract {
            returns(true) implies (this@Either is Left<A>)
            returns(false) implies (this@Either is Right<B>)
        }
        return this@Either is Left<A>
    }

    @OptIn(ExperimentalContracts::class)
    fun isRight(): Boolean {
        contract {
            returns(true) implies (this@Either is Right<B>)
            returns(false) implies (this@Either is Left<A>)
        }
        return this@Either is Right<B>
    }

    /**
     * Returns `false` if [Right]
     * or returns the result of the given [predicate] to the [Left] value.
     *
     * ```kotlin
     * import arrow.core.Either
     * import arrow.core.Either.Left
     * import arrow.core.Either.Right
     * import io.kotest.matchers.shouldBe
     *
     * fun test() {
     *  Left(12).isLeft { it > 10 } shouldBe true
     *  Left(7).isLeft { it > 10 } shouldBe false
     *
     *  val right: Either<Int, String> = Right("Hello World")
     *  right.isLeft { it > 10 } shouldBe false
     * }
     * ```
     * <!--- KNIT example-either-21.kt -->
     * <!--- TEST lines.isEmpty() -->
     */
    inline fun isLeft(predicate: (A) -> Boolean): Boolean {
        contract { returns(true) implies (this@Either is Left<A>) }
        return this@Either is Left<A> && predicate(value)
    }

    /**
     * Returns `false` if [Left]
     * or returns the result of the given [predicate] to the [Right] value.
     *
     * ```kotlin
     * import arrow.core.Either
     * import arrow.core.Either.Left
     * import arrow.core.Either.Right
     * import io.kotest.matchers.shouldBe
     *
     * fun test() {
     *  Right(12).isRight { it > 10 } shouldBe true
     *  Right(7).isRight { it > 10 } shouldBe false
     *
     *  val left: Either<String, Int> = Left("Hello World")
     *  left.isRight { it > 10 } shouldBe false
     * }
     * ```
     * <!--- KNIT example-either-22.kt -->
     * <!--- TEST lines.isEmpty() -->
     */
    inline fun isRight(predicate: (B) -> Boolean): Boolean {
        contract { returns(true) implies (this@Either is Right<B>) }
        return this@Either is Right<B> && predicate(value)
    }

    /**
     * Transform an [Either] into a value of [C].
     * Alternative to using `when` to fold an [Either] into a value [C].
     *
     * ```kotlin
     * import arrow.core.Either
     * import io.kotest.matchers.shouldBe
     * import io.kotest.assertions.fail
     *
     * fun test() {
     *   Either.Right(1)
     *     .fold({ fail("Cannot be left") }, { it + 1 }) shouldBe 2
     *
     *   Either.Left(RuntimeException("Boom!"))
     *     .fold({ -1 }, { fail("Cannot be right") }) shouldBe -1
     * }
     * ```
     * <!--- KNIT example-either-23.kt -->
     * <!--- TEST lines.isEmpty() -->
     *
     * @param ifLeft transform the [Either.Left] type [A] to [C].
     * @param ifRight transform the [Either.Right] type [B] to [C].
     * @return the transformed value [C] by applying [ifLeft] or [ifRight] to [A] or [B] respectively.
     */
    inline fun <C> fold(
        ifLeft: (left: A) -> C,
        ifRight: (right: B) -> C,
    ): C {
        contract {
            callsInPlace(ifLeft, InvocationKind.AT_MOST_ONCE)
            callsInPlace(ifRight, InvocationKind.AT_MOST_ONCE)
        }
        return when (this) {
            is Right -> ifRight(value)
            is Left -> ifLeft(value)
        }
    }

    /**
     * Swap the generic parameters [A] and [B] of this [Either].
     *
     * ```kotlin
     * import arrow.core.Either
     * import io.kotest.matchers.shouldBe
     *
     * fun test() {
     *   Either.Left("left").swap() shouldBe Either.Right("left")
     *   Either.Right("right").swap() shouldBe Either.Left("right")
     * }
     * ```
     * <!--- KNIT example-either-24.kt -->
     * <!-- TEST lines.isEmpty() -->
     */
    fun swap(): Either<B, A> = fold({ Right(it) }, { Left(it) })

    /**
     * Map, or transform, the right value [B] of this [Either] to a new value [C].
     *
     * ```kotlin
     * import arrow.core.Either
     * import io.kotest.matchers.shouldBe
     *
     * fun test() {
     *   Either.Right(12).map { _: Int ->"flower" } shouldBe Either.Right("flower")
     *   Either.Left(12).map { _: Nothing -> "flower" } shouldBe Either.Left(12)
     * }
     * ```
     * <!--- KNIT example-either-25.kt -->
     * <!--- TEST lines.isEmpty() -->
     */
    inline fun <C> map(f: (right: B) -> C): Either<A, C> {
        contract {
            callsInPlace(f, InvocationKind.AT_MOST_ONCE)
        }
        return flatMap { Right(f(it)) }
    }

    /**
     * Map, or transform, the left value [A] of this [Either] to a new value [C].
     *
     * ```kotlin
     * import arrow.core.Either
     * import io.kotest.matchers.shouldBe
     *
     * fun test() {
     *  Either.Right(12).mapLeft { _: Nothing -> "flower" } shouldBe Either.Right(12)
     *  Either.Left(12).mapLeft { _: Int -> "flower" }  shouldBe Either.Left("flower")
     * }
     * ```
     * <!--- KNIT example-either-26.kt -->
     * <!--- TEST lines.isEmpty() -->
     */
    inline fun <C> mapLeft(f: (A) -> C): Either<C, B> {
        contract {
            callsInPlace(f, InvocationKind.AT_MOST_ONCE)
        }
        return fold({ Left(f(it)) }, { Right(it) })
    }

    /**
     * Performs the given [action] on the encapsulated [B] value if this instance represents [Either.Right].
     * Returns the original [Either] unchanged.
     *
     * ```kotlin
     * import arrow.core.Either
     * import io.kotest.matchers.shouldBe
     *
     * fun test() {
     *   Either.Right(1).onRight(::println) shouldBe Either.Right(1)
     * }
     * ```
     * <!--- KNIT example-either-27.kt -->
     * <!--- TEST lines.isEmpty() -->
     */
    inline fun onRight(action: (right: B) -> Unit): Either<A, B> {
        contract {
            callsInPlace(action, InvocationKind.AT_MOST_ONCE)
        }
        return also { if (it.isRight()) action(it.value) }
    }

    /**
     * Performs the given [action] on the encapsulated [A] if this instance represents [Either.Left].
     * Returns the original [Either] unchanged.
     *
     * ```kotlin
     * import arrow.core.Either
     * import io.kotest.matchers.shouldBe
     *
     * fun test() {
     *   Either.Left(2).onLeft(::println) shouldBe Either.Left(2)
     * }
     * ```
     * <!--- KNIT example-either-28.kt -->
     * <!--- TEST lines.isEmpty() -->
     */
    inline fun onLeft(action: (left: A) -> Unit): Either<A, B> {
        contract {
            callsInPlace(action, InvocationKind.AT_MOST_ONCE)
        }
        return also { if (it.isLeft()) action(it.value) }
    }

    /**
     * Returns the unwrapped value [B] of [Either.Right] or `null` if it is [Either.Left].
     *
     * ```kotlin
     * import arrow.core.Either
     * import io.kotest.matchers.shouldBe
     *
     * fun test() {
     *   Either.Right(12).getOrNull() shouldBe 12
     *   Either.Left(12).getOrNull() shouldBe null
     * }
     * ```
     * <!--- KNIT example-either-30.kt -->
     * <!--- TEST lines.isEmpty() -->
     */
    fun getOrNull(): B? {
        contract {
            returns(null) implies (this@Either is Left<A>)
            returnsNotNull() implies (this@Either is Right<B>)
        }
        return getOrElse { null }
    }

    /**
     * Returns the unwrapped value [A] of [Either.Left] or `null` if it is [Either.Right].
     *
     * ```kotlin
     * import arrow.core.Either
     * import io.kotest.matchers.shouldBe
     *
     * fun test() {
     *   Either.Right(12).leftOrNull() shouldBe null
     *   Either.Left(12).leftOrNull() shouldBe 12
     * }
     * ```
     * <!--- KNIT example-either-31.kt -->
     * <!--- TEST lines.isEmpty() -->
     */
    fun leftOrNull(): A? {
        contract {
            returnsNotNull() implies (this@Either is Left<A>)
            returns(null) implies (this@Either is Right<B>)
        }

        return fold(::identity) { null }
    }

    override fun toString(): String =
        fold(
            { "Either.Left($it)" },
            { "Either.Right($it)" },
        )

    data class Left<out A>(
        val value: A,
    ) : Either<A, Nothing>() {
        override fun toString(): String = "Either.Left($value)"
    }

    data class Right<out B>(
        val value: B,
    ) : Either<Nothing, B>() {
        override fun toString(): String = "Either.Right($value)"
    }

    companion object {
        inline fun <E, A> catching(
            onCatch: (t: Throwable) -> E,
            block: () -> A,
        ): Either<E, A> {
            contract {
                callsInPlace(onCatch, InvocationKind.AT_MOST_ONCE)
                callsInPlace(block, InvocationKind.AT_MOST_ONCE)
            }
            return try {
                Right(block())
            } catch (t: Throwable) {
                logDebug("FEEDER_EITHER", "Catching caught exception", t)
                Left(onCatch(t))
            }
        }
    }
}

/**
 * Map, or transform, the right value [B] of this [Either] into a new [Either] with a right value of type [C].
 * Returns a new [Either] with either the original left value of type [A] or the newly transformed right value of type [C].
 *
 * @param f The function to bind across [Either.Right].
 */
inline fun <A, B, C> Either<A, B>.flatMap(f: (right: B) -> Either<A, C>): Either<A, C> {
    contract { callsInPlace(f, InvocationKind.AT_MOST_ONCE) }
    return when (this) {
        is Either.Right -> f(this.value)
        is Either.Left -> this
    }
}

fun <A, B> Either<A, Either<A, B>>.flatten(): Either<A, B> = flatMap(::identity)

/**
 * Get the right value [B] of this [Either],
 * or compute a [default] value with the left value [A].
 *
 * ```kotlin
 * import arrow.core.Either
 * import arrow.core.getOrElse
 * import io.kotest.matchers.shouldBe
 *
 * fun test() {
 *   Either.Left(12) getOrElse { it + 5 } shouldBe 17
 * }
 * ```
 * <!--- KNIT example-either-36.kt -->
 * <!--- TEST lines.isEmpty() -->
 */
inline infix fun <A, B> Either<A, B>.getOrElse(default: (A) -> B): B {
    contract { callsInPlace(default, InvocationKind.AT_MOST_ONCE) }
    return fold(default, ::identity)
}

/**
 * Returns the value from this [Either.Right] or [Either.Left].
 *
 * Example:
 * ```kotlin
 * import arrow.core.Either.Left
 * import arrow.core.Either.Right
 * import arrow.core.merge
 *
 * fun test() {
 *   Right(12).merge() // Result: 12
 *   Left(12).merge() // Result: 12
 * }
 * ```
 * <!--- KNIT example-either-41.kt -->
 * <!--- TEST lines.isEmpty() -->
 */
inline fun <A> Either<A, A>.merge(): A = fold(::identity, ::identity)

fun <A> A.left(): Either<A, Nothing> = Either.Left(this)

fun <A> A.right(): Either<Nothing, A> = Either.Right(this)

operator fun <A : Comparable<A>, B : Comparable<B>> Either<A, B>.compareTo(other: Either<A, B>): Int =
    fold(
        { a1 -> other.fold({ a2 -> a1.compareTo(a2) }, { -1 }) },
        { b1 -> other.fold({ 1 }, { b2 -> b1.compareTo(b2) }) },
    )

/**
 * Combine two [Either] values.
 * If both are [Either.Right] then combine both [B] values using [combineRight] or if both are [Either.Left] then combine both [A] values using [combineLeft],
 * otherwise it returns the `this` or fallbacks to [other] in case `this` is [Either.Left].
 */
fun <A, B> Either<A, B>.combine(
    other: Either<A, B>,
    combineLeft: (A, A) -> A,
    combineRight: (B, B) -> B,
): Either<A, B> =
    when (val one = this) {
        is Either.Left ->
            when (other) {
                is Either.Left -> Either.Left(combineLeft(one.value, other.value))
                is Either.Right -> one
            }

        is Either.Right ->
            when (other) {
                is Either.Left -> other
                is Either.Right -> Either.Right(combineRight(one.value, other.value))
            }
    }

/**
 * Given [B] is a sub type of [C], re-type this value from Either<A, B> to Either<A, C>
 *
 * ```kotlin
 * import arrow.core.*
 *
 * fun main(args: Array<String>) {
 *   //sampleStart
 *   val string: Either<Int, String> = "Hello".right()
 *   val chars: Either<Int, CharSequence> =
 *     string.widen<Int, CharSequence, String>()
 *   //sampleEnd
 *   println(chars)
 * }
 * ```
 * <!--- KNIT example-either-44.kt -->
 */
fun <A, C, B : C> Either<A, B>.widen(): Either<A, C> = this

fun <AA, A : AA, B> Either<A, B>.leftWiden(): Either<AA, B> = this

fun <A> identity(value: (A)): A = value
