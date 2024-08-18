package moe.matsuri.nb4a.proxy.neko;

import androidx.annotation.NonNull;

import com.esotericsoftware.kryo.io.ByteBufferInput;
import com.esotericsoftware.kryo.io.ByteBufferOutput;

import org.jetbrains.annotations.NotNull;
import org.json.JSONObject;

import io.nekohasekai.sagernet.fmt.AbstractBean;
import io.nekohasekai.sagernet.fmt.KryoConverters;
import io.nekohasekai.sagernet.ktx.Logs;
import moe.matsuri.nb4a.plugin.NekoPluginManager;

public class NekoBean extends AbstractBean {

    // BoxInstance use this
    public JSONObject allConfig = null;

    public String plgId;
    public String protocolId;
    public JSONObject sharedStorage = new JSONObject();

    @Override
    public void initializeDefaultValues() {
        super.initializeDefaultValues();
        if (protocolId == null) protocolId = "";
        if (plgId == null) plgId = "moe.matsuri.plugin.donotexist";
    }

    @Override
    public void serialize(ByteBufferOutput output) {
        output.writeInt(0);
        super.serialize(output);
        output.writeString(plgId);
        output.writeString(protocolId);
        output.writeString(sharedStorage.toString());
    }

    @Override
    public void deserialize(ByteBufferInput input) {
        int version = input.readInt();
        super.deserialize(input);
        plgId = input.readString();
        protocolId = input.readString();
        sharedStorage = tryParseJSON(input.readString());
    }

    @NotNull
    public static JSONObject tryParseJSON(String input) {
        JSONObject ret;
        try {
            ret = new JSONObject(input);
        } catch (Exception e) {
            ret = new JSONObject();
            Logs.INSTANCE.e(e);
        }
        return ret;
    }

    public String displayType() {
        NekoPluginManager.Protocol p = NekoPluginManager.INSTANCE.findProtocol(protocolId);
        String neko =  "Advanced plugin";
        if (p == null) return neko;
        return p.getProtocolId();
    }

    @Override
    public boolean canMapping() {
        NekoPluginManager.Protocol p = NekoPluginManager.INSTANCE.findProtocol(protocolId);
        if (p == null) return false;
        return p.getProtocolConfig().optBoolean("canMapping");
    }

    @Override
    public boolean canICMPing() {
        NekoPluginManager.Protocol p = NekoPluginManager.INSTANCE.findProtocol(protocolId);
        if (p == null) return false;
        return p.getProtocolConfig().optBoolean("canICMPing");
    }

    @Override
    public boolean canTCPing() {
        NekoPluginManager.Protocol p = NekoPluginManager.INSTANCE.findProtocol(protocolId);
        if (p == null) return false;
        return p.getProtocolConfig().optBoolean("canTCPing");
    }

    @NotNull
    @Override
    public NekoBean clone() {
        return KryoConverters.deserialize(new NekoBean(), KryoConverters.serialize(this));
    }

    public static final Creator<NekoBean> CREATOR = new CREATOR<NekoBean>() {
        @NonNull
        @Override
        public NekoBean newInstance() {
            return new NekoBean();
        }

        @Override
        public NekoBean[] newArray(int size) {
            return new NekoBean[size];
        }
    };
}
