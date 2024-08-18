package io.nekohasekai.sagernet.utils.cf

import com.google.gson.Gson

data class UpdateDeviceRequest(
    var name: String, var active: Boolean
) {
    companion object {
        fun newRequest(name: String = "SagerNet Client", active: Boolean = true) =
            Gson().toJson(UpdateDeviceRequest(name, active))
    }
}