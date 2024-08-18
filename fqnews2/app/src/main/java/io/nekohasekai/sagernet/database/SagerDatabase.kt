package io.nekohasekai.sagernet.database

import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase
import androidx.room.TypeConverters
import jww.app.FeederApplication
import dev.matrix.roomigrant.GenerateRoomMigrations
import io.nekohasekai.sagernet.Key

import io.nekohasekai.sagernet.fmt.KryoConverters
import io.nekohasekai.sagernet.fmt.gson.GsonConverters
import kotlinx.coroutines.DelicateCoroutinesApi
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch

@Database(
    entities = [ProxyGroup::class, ProxyEntity::class, RuleEntity::class],
    version = 3
)
@TypeConverters(value = [KryoConverters::class, GsonConverters::class])
@GenerateRoomMigrations
abstract class SagerDatabase : RoomDatabase() {

    companion object {
        @OptIn(DelicateCoroutinesApi::class)
        @Suppress("EXPERIMENTAL_API_USAGE")
        val instance by lazy {
            FeederApplication.instance.getDatabasePath(Key.DB_PROFILE).parentFile?.mkdirs()
            Room.databaseBuilder(FeederApplication.instance, SagerDatabase::class.java, Key.DB_PROFILE)
                //.addMigrations(*SagerDatabase_Migrations.build())
                .allowMainThreadQueries()
                .enableMultiInstanceInvalidation()
                .fallbackToDestructiveMigration()
                .setQueryExecutor { GlobalScope.launch { it.run() } }
                .build()
        }

        val groupDao get() = instance.groupDao()
        val proxyDao get() = instance.proxyDao()
        val rulesDao get() = instance.rulesDao()

    }

    abstract fun groupDao(): ProxyGroup.Dao
    abstract fun proxyDao(): ProxyEntity.Dao
    abstract fun rulesDao(): RuleEntity.Dao

}
