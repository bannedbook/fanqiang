package io.nekohasekai.sagernet.fmt.trojan_go;

import androidx.annotation.NonNull;

import com.esotericsoftware.kryo.io.ByteBufferInput;
import com.esotericsoftware.kryo.io.ByteBufferOutput;

import org.jetbrains.annotations.NotNull;

import io.nekohasekai.sagernet.fmt.AbstractBean;
import io.nekohasekai.sagernet.fmt.KryoConverters;
import moe.matsuri.nb4a.utils.JavaUtil;

public class TrojanGoBean extends AbstractBean {

    /**
     * Trojan 的密码。
     * 不可省略，不能为空字符串，不建议含有非 ASCII 可打印字符。
     * 必须使用 encodeURIComponent 编码。
     */
    public String password;

    /**
     * 自定义 TLS 的 SNI。
     * 省略时默认与 trojan-host 同值。不得为空字符串。
     * <p>
     * 必须使用 encodeURIComponent 编码。
     */
    public String sni;

    /**
     * 传输类型。
     * 省略时默认为 original，但不可为空字符串。
     * 目前可选值只有 original 和 ws，未来可能会有 h2、h2+ws 等取值。
     * <p>
     * 当取值为 original 时，使用原始 Trojan 传输方式，无法方便通过 CDN。
     * 当取值为 ws 时，使用 wss 作为传输层。
     */
    public String type;

    /**
     * 自定义 HTTP Host 头。
     * 可以省略，省略时值同 trojan-host。
     * 可以为空字符串，但可能带来非预期情形。
     * <p>
     * 警告：若你的端口非标准端口（不是 80 / 443），RFC 标准规定 Host 应在主机名后附上端口号，例如 example.com:44333。至于是否遵守，请自行斟酌。
     * <p>
     * 必须使用 encodeURIComponent 编码。
     */
    public String host;

    /**
     * 当传输类型 type 取 ws、h2、h2+ws 时，此项有效。
     * 不可省略，不可为空。
     * 必须以 / 开头。
     * 可以使用 URL 中的 & # ? 等字符，但应当是合法的 URL 路径。
     * <p>
     * 必须使用 encodeURIComponent 编码。
     */
    public String path;

    /**
     * 用于保证 Trojan 流量密码学安全的加密层。
     * 可省略，默认为 none，即不使用加密。
     * 不可以为空字符串。
     * <p>
     * 必须使用 encodeURIComponent 编码。
     * <p>
     * 使用 Shadowsocks 算法进行流量加密时，其格式为：
     * <p>
     * ss;method:password
     * <p>
     * 其中 ss 是固定内容，method 是加密方法，必须为下列之一：
     * <p>
     * aes-128-gcm
     * aes-256-gcm
     * chacha20-ietf-poly1305
     */
    public String encryption;

    /**
     * 额外的插件选项。本字段保留。
     * 可省略，但不可以为空字符串。
     */
    // not used in NB4A
    public String plugin;

    // ---

    public Boolean allowInsecure;

    @Override
    public void initializeDefaultValues() {
        super.initializeDefaultValues();

        if (password == null) password = "";
        if (sni == null) sni = "";
        if (JavaUtil.isNullOrBlank(type)) type = "original";
        if (host == null) host = "";
        if (path == null) path = "";
        if (JavaUtil.isNullOrBlank(encryption)) encryption = "none";
        if (plugin == null) plugin = "";
        if (allowInsecure == null) allowInsecure = false;
    }

    @Override
    public void serialize(ByteBufferOutput output) {
        output.writeInt(1);
        super.serialize(output);
        output.writeString(password);
        output.writeString(sni);
        output.writeString(type);
        //noinspection SwitchStatementWithTooFewBranches
        switch (type) {
            case "ws": {
                output.writeString(host);
                output.writeString(path);
                break;
            }
        }
        output.writeString(encryption);
        output.writeString(plugin);
        output.writeBoolean(allowInsecure);
    }

    @Override
    public void deserialize(ByteBufferInput input) {
        int version = input.readInt();
        super.deserialize(input);

        password = input.readString();
        sni = input.readString();
        type = input.readString();
        //noinspection SwitchStatementWithTooFewBranches
        switch (type) {
            case "ws": {
                host = input.readString();
                path = input.readString();
                break;
            }
        }
        encryption = input.readString();
        plugin = input.readString();
        if (version >= 1) {
            allowInsecure = input.readBoolean();
        }
    }

    @Override
    public void applyFeatureSettings(AbstractBean other) {
        if (!(other instanceof TrojanGoBean)) return;
        TrojanGoBean bean = ((TrojanGoBean) other);
        if (allowInsecure) {
            bean.allowInsecure = true;
        }
    }

    @NotNull
    @Override
    public TrojanGoBean clone() {
        return KryoConverters.deserialize(new TrojanGoBean(), KryoConverters.serialize(this));
    }

    public static final Creator<TrojanGoBean> CREATOR = new CREATOR<TrojanGoBean>() {
        @NonNull
        @Override
        public TrojanGoBean newInstance() {
            return new TrojanGoBean();
        }

        @Override
        public TrojanGoBean[] newArray(int size) {
            return new TrojanGoBean[size];
        }
    };
}
