package io.nekohasekai.sagernet.fmt.gson;

import androidx.room.TypeConverter;

import java.util.Collection;
import java.util.List;
import java.util.Set;

import kotlin.collections.CollectionsKt;
import kotlin.collections.SetsKt;
import moe.matsuri.nb4a.utils.JavaUtil;

public class GsonConverters {

    @TypeConverter
    public static String toJson(Object value) {
        if (value instanceof Collection) {
            if (((Collection<?>) value).isEmpty()) return "";
        }
        return JavaUtil.gson.toJson(value);
    }

    @TypeConverter
    public static List toList(String value) {
        if (JavaUtil.isNullOrBlank(value)) return CollectionsKt.listOf();
        return JavaUtil.gson.fromJson(value, List.class);
    }

    @TypeConverter
    public static Set toSet(String value) {
        if (JavaUtil.isNullOrBlank(value)) return SetsKt.setOf();
        return JavaUtil.gson.fromJson(value, Set.class);
    }

}
