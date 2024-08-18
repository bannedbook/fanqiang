package io.nekohasekai.sagernet.fmt.internal;

import androidx.annotation.NonNull;

import com.esotericsoftware.kryo.io.ByteBufferInput;
import com.esotericsoftware.kryo.io.ByteBufferOutput;

import org.jetbrains.annotations.NotNull;

import java.util.ArrayList;
import java.util.List;

import io.nekohasekai.sagernet.fmt.KryoConverters;
import moe.matsuri.nb4a.utils.JavaUtil;

public class ChainBean extends InternalBean {

    public List<Long> proxies;

    @Override
    public String displayName() {
        if (JavaUtil.isNotBlank(name)) {
            return name;
        } else {
            return "Chain " + Math.abs(hashCode());
        }
    }

    @Override
    public void initializeDefaultValues() {
        super.initializeDefaultValues();
        if (name == null) name = "";

        if (proxies == null) {
            proxies = new ArrayList<>();
        }
    }

    @Override
    public void serialize(ByteBufferOutput output) {
        output.writeInt(1);
        output.writeInt(proxies.size());
        for (Long proxy : proxies) {
            output.writeLong(proxy);
        }
    }

    @Override
    public void deserialize(ByteBufferInput input) {
        int version = input.readInt();
        if (version < 1) {
            input.readString();
            input.readInt();
        }
        int length = input.readInt();
        proxies = new ArrayList<>();
        for (int i = 0; i < length; i++) {
            proxies.add(input.readLong());
        }
    }

    @NotNull
    @Override
    public ChainBean clone() {
        return KryoConverters.deserialize(new ChainBean(), KryoConverters.serialize(this));
    }

    public static final Creator<ChainBean> CREATOR = new CREATOR<ChainBean>() {
        @NonNull
        @Override
        public ChainBean newInstance() {
            return new ChainBean();
        }

        @Override
        public ChainBean[] newArray(int size) {
            return new ChainBean[size];
        }
    };
}
