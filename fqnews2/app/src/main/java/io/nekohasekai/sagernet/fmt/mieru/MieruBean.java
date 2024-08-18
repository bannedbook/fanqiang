/******************************************************************************
 * Copyright (C) 2022 by nekohasekai <contact-git@sekai.icu>                  *
 *                                                                            *
 * This program is free software: you can redistribute it and/or modify       *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 *  (at your option) any later version.                                       *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.       *
 *                                                                            *
 ******************************************************************************/

package io.nekohasekai.sagernet.fmt.mieru;

import androidx.annotation.NonNull;

import com.esotericsoftware.kryo.io.ByteBufferInput;
import com.esotericsoftware.kryo.io.ByteBufferOutput;

import org.jetbrains.annotations.NotNull;

import io.nekohasekai.sagernet.fmt.AbstractBean;
import io.nekohasekai.sagernet.fmt.KryoConverters;

public class MieruBean extends AbstractBean {

    public String protocol;
    public String username;
    public String password;
    public Integer mtu;

    @Override
    public void initializeDefaultValues() {
        super.initializeDefaultValues();
        if (protocol == null) protocol = "TCP";
        if (username == null) username = "";
        if (password == null) password = "";
        if (mtu == null) mtu = 1400;
    }

    @Override
    public void serialize(ByteBufferOutput output) {
        output.writeInt(0);
        super.serialize(output);
        output.writeString(protocol);
        output.writeString(username);
        output.writeString(password);
        if (protocol.equals("UDP")) {
            output.writeInt(mtu);
        }
    }

    @Override
    public void deserialize(ByteBufferInput input) {
        int version = input.readInt();
        super.deserialize(input);
        protocol = input.readString();
        username = input.readString();
        password = input.readString();
        if (protocol.equals("UDP")) {
            mtu = input.readInt();
        }
    }

    @NotNull
    @Override
    public MieruBean clone() {
        return KryoConverters.deserialize(new MieruBean(), KryoConverters.serialize(this));
    }

    public static final Creator<MieruBean> CREATOR = new CREATOR<MieruBean>() {
        @NonNull
        @Override
        public MieruBean newInstance() {
            return new MieruBean();
        }

        @Override
        public MieruBean[] newArray(int size) {
            return new MieruBean[size];
        }
    };
}
