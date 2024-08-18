package io.nekohasekai.sagernet.database;

import androidx.annotation.NonNull;

import com.esotericsoftware.kryo.io.ByteBufferInput;
import com.esotericsoftware.kryo.io.ByteBufferOutput;

import java.util.ArrayList;
import java.util.List;

import io.nekohasekai.sagernet.fmt.Serializable;

public class SubscriptionBean extends Serializable {

    public Integer type;
    public String link;
    public String token;
    public Boolean forceResolve;
    public Boolean deduplication;
    public Boolean updateWhenConnectedOnly;
    public String customUserAgent;
    public Boolean autoUpdate;
    public Integer autoUpdateDelay;
    public Integer lastUpdated;

    // SIP008

    public Long bytesUsed;
    public Long bytesRemaining;

    // Open Online Config

    public String username;
    public Integer expiryDate;
    public List<String> protocols;


    // https://github.com/crossutility/Quantumult/blob/master/extra-subscription-feature.md

    public String subscriptionUserinfo;

    public SubscriptionBean() {
    }

    @Override
    public void serializeToBuffer(ByteBufferOutput output) {
        output.writeInt(1);

        output.writeInt(type);

        output.writeString(link);

        output.writeBoolean(forceResolve);
        output.writeBoolean(deduplication);
        output.writeBoolean(updateWhenConnectedOnly);
        output.writeString(customUserAgent);
        output.writeBoolean(autoUpdate);
        output.writeInt(autoUpdateDelay);
        output.writeInt(lastUpdated);

        output.writeString(subscriptionUserinfo);
    }

    public void serializeForShare(ByteBufferOutput output) {
        output.writeInt(0);

        output.writeInt(type);

        output.writeString(link);

        output.writeBoolean(forceResolve);
        output.writeBoolean(deduplication);
        output.writeBoolean(updateWhenConnectedOnly);
        output.writeString(customUserAgent);
    }

    @Override
    public void deserializeFromBuffer(ByteBufferInput input) {
        int version = input.readInt();

        type = input.readInt();
        link = input.readString();
        forceResolve = input.readBoolean();
        deduplication = input.readBoolean();
        updateWhenConnectedOnly = input.readBoolean();
        customUserAgent = input.readString();
        autoUpdate = input.readBoolean();
        autoUpdateDelay = input.readInt();
        lastUpdated = input.readInt();
        subscriptionUserinfo = input.readString();
    }

    public void deserializeFromShare(ByteBufferInput input) {
        int version = input.readInt();

        type = input.readInt();
        link = input.readString();
        forceResolve = input.readBoolean();
        deduplication = input.readBoolean();
        updateWhenConnectedOnly = input.readBoolean();
        customUserAgent = input.readString();
    }

    @Override
    public void initializeDefaultValues() {
        if (type == null) type = 0;
        if (link == null) link = "";
        if (token == null) token = "";
        if (forceResolve == null) forceResolve = false;
        if (deduplication == null) deduplication = false;
        if (updateWhenConnectedOnly == null) updateWhenConnectedOnly = false;
        if (customUserAgent == null) customUserAgent = "";
        if (autoUpdate == null) autoUpdate = false;
        if (autoUpdateDelay == null) autoUpdateDelay = 1440;
        if (lastUpdated == null) lastUpdated = 0;

        if (bytesUsed == null) bytesUsed = 0L;
        if (bytesRemaining == null) bytesRemaining = 0L;

        if (username == null) username = "";
        if (expiryDate == null) expiryDate = 0;
        if (protocols == null) protocols = new ArrayList<>();
    }

    public static final Creator<SubscriptionBean> CREATOR = new CREATOR<SubscriptionBean>() {
        @NonNull
        @Override
        public SubscriptionBean newInstance() {
            return new SubscriptionBean();
        }

        @Override
        public SubscriptionBean[] newArray(int size) {
            return new SubscriptionBean[size];
        }
    };

}
