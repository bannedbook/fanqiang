package com.nononsenseapps.feeder.util

@RequiresOptIn("This is only for testing/migration and should not be used in production code.")
@Retention(AnnotationRetention.BINARY)
@Target(AnnotationTarget.CLASS, AnnotationTarget.FUNCTION)
annotation class DoNotUseInProd
