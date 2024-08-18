package io.nekohasekai.sagernet.fmt.v2ray;

import com.esotericsoftware.kryo.io.ByteBufferInput;
import com.esotericsoftware.kryo.io.ByteBufferOutput;

import io.nekohasekai.sagernet.fmt.AbstractBean;
import io.nekohasekai.sagernet.fmt.trojan.TrojanBean;
import moe.matsuri.nb4a.utils.JavaUtil;

public abstract class StandardV2RayBean extends AbstractBean {

    public String uuid;
    public String encryption; // or VLESS flow

    //////// End of VMess & VLESS ////////

    // "V2Ray Transport" tcp/http/ws/quic/grpc/httpUpgrade
    public String type;

    public String host;

    public String path;

    // --------------------------------------- tls?

    public String security;

    public String sni;

    public String alpn;

    public String utlsFingerprint;

    public Boolean allowInsecure;

    // --------------------------------------- reality


    public String realityPubKey;

    public String realityShortId;


    // --------------------------------------- //

    public Integer wsMaxEarlyData;
    public String earlyDataHeaderName;

    public String certificates;

    // --------------------------------------- ech

    public Boolean enableECH;

    public Boolean enablePqSignature;

    public Boolean disabledDRS;

    public String echConfig;

    // --------------------------------------- //

    public Integer packetEncoding; // 1:packet 2:xudp

    @Override
    public void initializeDefaultValues() {
        super.initializeDefaultValues();

        if (JavaUtil.isNullOrBlank(uuid)) uuid = "";

        if (JavaUtil.isNullOrBlank(type)) type = "tcp";
        else if ("h2".equals(type)) type = "http";

        type = type.toLowerCase();

        if (JavaUtil.isNullOrBlank(host)) host = "";
        if (JavaUtil.isNullOrBlank(path)) path = "";

        if (JavaUtil.isNullOrBlank(security)) {
            if (this instanceof TrojanBean || isVLESS()) {
                security = "tls";
            } else {
                security = "none";
            }
        }
        if (JavaUtil.isNullOrBlank(sni)) sni = "";
        if (JavaUtil.isNullOrBlank(alpn)) alpn = "";

        if (JavaUtil.isNullOrBlank(certificates)) certificates = "";
        if (JavaUtil.isNullOrBlank(earlyDataHeaderName)) earlyDataHeaderName = "";
        if (JavaUtil.isNullOrBlank(utlsFingerprint)) utlsFingerprint = "";

        if (wsMaxEarlyData == null) wsMaxEarlyData = 0;
        if (allowInsecure == null) allowInsecure = false;
        if (packetEncoding == null) packetEncoding = 0;

        if (realityPubKey == null) realityPubKey = "";
        if (realityShortId == null) realityShortId = "";

        if (enableECH == null) enableECH = false;
        if (JavaUtil.isNullOrBlank(echConfig)) echConfig = "";
        if (enablePqSignature == null) enablePqSignature = false;
        if (disabledDRS == null) disabledDRS = false;
    }

    @Override
    public void serialize(ByteBufferOutput output) {
        output.writeInt(1);
        super.serialize(output);
        output.writeString(uuid);
        output.writeString(encryption);
        if (this instanceof VMessBean) {
            output.writeInt(((VMessBean) this).alterId);
        }

        output.writeString(type);
        switch (type) {
            case "tcp":
            case "quic": {
                break;
            }
            case "ws": {
                output.writeString(host);
                output.writeString(path);
                output.writeInt(wsMaxEarlyData);
                output.writeString(earlyDataHeaderName);
                break;
            }
            case "http": {
                output.writeString(host);
                output.writeString(path);
                break;
            }
            case "grpc": {
                output.writeString(path);
            }
            case "httpupgrade": {
                output.writeString(host);
                output.writeString(path);

            }
        }

        output.writeString(security);
        if ("tls".equals(security)) {
            output.writeString(sni);
            output.writeString(alpn);
            output.writeString(certificates);
            output.writeBoolean(allowInsecure);
            output.writeString(utlsFingerprint);
            output.writeString(realityPubKey);
            output.writeString(realityShortId);
        }

        output.writeBoolean(enableECH);
        if (enableECH) {
            output.writeBoolean(enablePqSignature);
            output.writeBoolean(disabledDRS);
            output.writeString(echConfig);
        }

        output.writeInt(packetEncoding);
    }

    @Override
    public void deserialize(ByteBufferInput input) {
        int version = input.readInt();
        super.deserialize(input);
        uuid = input.readString();
        encryption = input.readString();
        if (this instanceof VMessBean) {
            ((VMessBean) this).alterId = input.readInt();
        }

        type = input.readString();
        switch (type) {
            case "tcp":
            case "quic": {
                break;
            }
            case "ws": {
                host = input.readString();
                path = input.readString();
                wsMaxEarlyData = input.readInt();
                earlyDataHeaderName = input.readString();
                break;
            }
            case "http": {
                host = input.readString();
                path = input.readString();
                break;
            }
            case "grpc": {
                path = input.readString();
            }
            case "httpupgrade": {
                host = input.readString();
                path = input.readString();
            }
        }

        security = input.readString();
        if ("tls".equals(security)) {
            sni = input.readString();
            alpn = input.readString();
            certificates = input.readString();
            allowInsecure = input.readBoolean();
            utlsFingerprint = input.readString();
            realityPubKey = input.readString();
            realityShortId = input.readString();
        }

        if (version >= 1) { // 从老版本升级上来
            enableECH = input.readBoolean();
            if (enableECH) {
                enablePqSignature = input.readBoolean();
                disabledDRS = input.readBoolean();
                echConfig = input.readString();
            }
        }

        if (version == 0) {
            // 从老版本升级上来但是 version == 0, 可能有 enableECH 也可能没有，需要做判断
            int position = input.getByteBuffer().position(); // 当前位置

            boolean tmpEnableECH = input.readBoolean();
            int tmpPacketEncoding = input.readInt();

            input.setPosition(position); // 读后归位

            if (tmpPacketEncoding != 1 && tmpPacketEncoding != 2) {
                input.getByteBuffer().position(position);
                enableECH = tmpEnableECH;
                if (enableECH) {
                    enablePqSignature = input.readBoolean();
                    disabledDRS = input.readBoolean();
                    echConfig = input.readString();
                }
            } // 否则后一位就是 packetEncoding
        }

        packetEncoding = input.readInt();
    }

    @Override
    public void applyFeatureSettings(AbstractBean other) {
        if (!(other instanceof StandardV2RayBean)) return;
        StandardV2RayBean bean = ((StandardV2RayBean) other);
        bean.allowInsecure = allowInsecure;
        bean.utlsFingerprint = utlsFingerprint;
        bean.packetEncoding = packetEncoding;
        bean.enableECH = enableECH;
        bean.disabledDRS = disabledDRS;
        bean.echConfig = echConfig;
    }

    public boolean isVLESS() {
        if (this instanceof VMessBean) {
            Integer aid = ((VMessBean) this).alterId;
            return aid != null && aid == -1;
        }
        return false;
    }

}
