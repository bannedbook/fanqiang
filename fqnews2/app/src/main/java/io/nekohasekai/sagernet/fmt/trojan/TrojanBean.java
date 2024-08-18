package io.nekohasekai.sagernet.fmt.trojan;

import androidx.annotation.NonNull;

import com.esotericsoftware.kryo.io.ByteBufferInput;
import com.esotericsoftware.kryo.io.ByteBufferOutput;

import org.jetbrains.annotations.NotNull;

import io.nekohasekai.sagernet.fmt.KryoConverters;
import io.nekohasekai.sagernet.fmt.v2ray.StandardV2RayBean;

public class TrojanBean extends StandardV2RayBean {

    public String password;

    @Override
    public void initializeDefaultValues() {
        super.initializeDefaultValues();
        if (security == null || security.isEmpty()) security = "tls";
        if (password == null) password = "";
    }

    @Override
    public void serialize(ByteBufferOutput output) {
        output.writeInt(2);
        super.serialize(output);
        output.writeString(password);
    }

    @Override
    public void deserialize(ByteBufferInput input) {
        int version = input.readInt();
        if (version >= 2) {
            super.deserialize(input); // StandardV2RayBean
            password = input.readString();
        } else {
            // From AbstractBean
            serverAddress = input.readString();
            serverPort = input.readInt();
            // From TrojanBean
            password = input.readString();
            security = input.readString();
            sni = input.readString();
            alpn = input.readString();
            if (version == 1) allowInsecure = input.readBoolean();
        }
    }

    @NotNull
    @Override
    public TrojanBean clone() {
        return KryoConverters.deserialize(new TrojanBean(), KryoConverters.serialize(this));
    }

    public static final Creator<TrojanBean> CREATOR = new CREATOR<TrojanBean>() {
        @NonNull
        @Override
        public TrojanBean newInstance() {
            return new TrojanBean();
        }

        @Override
        public TrojanBean[] newArray(int size) {
            return new TrojanBean[size];
        }
    };
}
