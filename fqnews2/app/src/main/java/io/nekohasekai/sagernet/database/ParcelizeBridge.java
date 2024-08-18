package io.nekohasekai.sagernet.database;

import android.os.Parcel;

/**
 * see: https://youtrack.jetbrains.com/issue/KT-19853
 */
public class ParcelizeBridge {

    public static RuleEntity createRule(Parcel parcel) {
        return (RuleEntity) RuleEntity.CREATOR.createFromParcel(parcel);
    }
}
