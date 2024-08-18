package moe.matsuri.nb4a.proxy.config;

import androidx.annotation.NonNull;

import com.esotericsoftware.kryo.io.ByteBufferInput;
import com.esotericsoftware.kryo.io.ByteBufferOutput;

import org.jetbrains.annotations.NotNull;

import io.nekohasekai.sagernet.fmt.KryoConverters;
import io.nekohasekai.sagernet.fmt.internal.InternalBean;
import moe.matsuri.nb4a.utils.JavaUtil;

public class ConfigBean extends InternalBean {

    public Integer type; // 0=config 1=outbound
    public String config;

    @Override
    public void initializeDefaultValues() {
        super.initializeDefaultValues();
        if (type == null) type = 0;
        if (config == null) config = "";
    }

    @Override
    public void serialize(ByteBufferOutput output) {
        output.writeInt(0);
        super.serialize(output);
        output.writeInt(type);
        output.writeString(config);
    }

    @Override
    public void deserialize(ByteBufferInput input) {
        int version = input.readInt();
        super.deserialize(input);
        type = input.readInt();
        config = input.readString();
    }

    @Override
    public String displayName() {
        if (JavaUtil.isNotBlank(name)) {
            return name;
        } else {
            return "Custom " + Math.abs(hashCode());
        }
    }

    public String displayType() {
        return type == 0 ? "sing-box config" : "sing-box outbound";
    }

    @NotNull
    @Override
    public ConfigBean clone() {
        return KryoConverters.deserialize(new ConfigBean(), KryoConverters.serialize(this));
    }

    public static final Creator<ConfigBean> CREATOR = new CREATOR<ConfigBean>() {
        @NonNull
        @Override
        public ConfigBean newInstance() {
            return new ConfigBean();
        }

        @Override
        public ConfigBean[] newArray(int size) {
            return new ConfigBean[size];
        }
    };
}