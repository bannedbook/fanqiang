package io.nekohasekai.sagernet.fmt.ssh;

import androidx.annotation.NonNull;

import com.esotericsoftware.kryo.io.ByteBufferInput;
import com.esotericsoftware.kryo.io.ByteBufferOutput;

import org.jetbrains.annotations.NotNull;

import io.nekohasekai.sagernet.fmt.AbstractBean;
import io.nekohasekai.sagernet.fmt.KryoConverters;

public class SSHBean extends AbstractBean {

    public static final int AUTH_TYPE_NONE = 0;
    public static final int AUTH_TYPE_PASSWORD = 1;
    public static final int AUTH_TYPE_PRIVATE_KEY = 2;

    public String username;
    public Integer authType;
    public String password;
    public String privateKey;
    public String privateKeyPassphrase;
    public String publicKey;

    @Override
    public void initializeDefaultValues() {
        if (serverPort == null) serverPort = 22;

        super.initializeDefaultValues();

        if (username == null) username = "root";
        if (authType == null) authType = AUTH_TYPE_PASSWORD;
        if (password == null) password = "";
        if (privateKey == null) privateKey = "";
        if (privateKeyPassphrase == null) privateKeyPassphrase = "";
        if (publicKey == null) publicKey = "";
    }

    @Override
    public void serialize(ByteBufferOutput output) {
        output.writeInt(0);
        super.serialize(output);
        output.writeString(username);
        output.writeInt(authType);
        switch (authType) {
            case AUTH_TYPE_NONE:
                break;
            case AUTH_TYPE_PASSWORD:
                output.writeString(password);
                break;
            case AUTH_TYPE_PRIVATE_KEY:
                output.writeString(privateKey);
                output.writeString(privateKeyPassphrase);
                break;
        }
        output.writeString(publicKey);
    }

    @Override
    public void deserialize(ByteBufferInput input) {
        int version = input.readInt();
        super.deserialize(input);
        username = input.readString();
        authType = input.readInt();
        switch (authType) {
            case AUTH_TYPE_NONE:
                break;
            case AUTH_TYPE_PASSWORD:
                password = input.readString();
                break;
            case AUTH_TYPE_PRIVATE_KEY:
                privateKey = input.readString();
                privateKeyPassphrase = input.readString();
                break;
        }
        publicKey = input.readString();
    }

    @NotNull
    @Override
    public SSHBean clone() {
        return KryoConverters.deserialize(new SSHBean(), KryoConverters.serialize(this));
    }

    public static final Creator<SSHBean> CREATOR = new CREATOR<SSHBean>() {
        @NonNull
        @Override
        public SSHBean newInstance() {
            return new SSHBean();
        }

        @Override
        public SSHBean[] newArray(int size) {
            return new SSHBean[size];
        }
    };
}
