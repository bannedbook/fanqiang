package io.nekohasekai.sagernet.utils.cf


import com.google.gson.annotations.SerializedName

data class DeviceResponse(
    @SerializedName("created")
    var created: String = "",
    @SerializedName("type")
    var type: String = "",
    @SerializedName("locale")
    var locale: String = "",
    @SerializedName("enabled")
    var enabled: Boolean = false,
    @SerializedName("token")
    var token: String = "",
    @SerializedName("waitlist_enabled")
    var waitlistEnabled: Boolean = false,
    @SerializedName("install_id")
    var installId: String = "",
    @SerializedName("warp_enabled")
    var warpEnabled: Boolean = false,
    @SerializedName("name")
    var name: String = "",
    @SerializedName("fcm_token")
    var fcmToken: String = "",
    @SerializedName("tos")
    var tos: String = "",
    @SerializedName("model")
    var model: String = "",
    @SerializedName("id")
    var id: String = "",
    @SerializedName("place")
    var place: Int = 0,
    @SerializedName("config")
    var config: Config = Config(),
    @SerializedName("updated")
    var updated: String = "",
    @SerializedName("key")
    var key: String = "",
    @SerializedName("account")
    var account: Account = Account()
) {
    data class Config(
        @SerializedName("peers")
        var peers: List<Peer> = listOf(),
        @SerializedName("services")
        var services: Services = Services(),
        @SerializedName("interface")
        var interfaceX: Interface = Interface(),
        @SerializedName("client_id")
        var clientId: String = ""
    ) {
        data class Peer(
            @SerializedName("public_key")
            var publicKey: String = "",
            @SerializedName("endpoint")
            var endpoint: Endpoint = Endpoint()
        ) {
            data class Endpoint(
                @SerializedName("v6")
                var v6: String = "",
                @SerializedName("host")
                var host: String = "",
                @SerializedName("v4")
                var v4: String = ""
            )
        }

        data class Services(
            @SerializedName("http_proxy")
            var httpProxy: String = ""
        )

        data class Interface(
            @SerializedName("addresses")
            var addresses: Addresses = Addresses()
        ) {
            data class Addresses(
                @SerializedName("v6")
                var v6: String = "",
                @SerializedName("v4")
                var v4: String = ""
            )
        }
    }

    data class Account(
        @SerializedName("account_type")
        var accountType: String = "",
        @SerializedName("role")
        var role: String = "",
        @SerializedName("referral_renewal_countdown")
        var referralRenewalCountdown: Int = 0,
        @SerializedName("created")
        var created: String = "",
        @SerializedName("usage")
        var usage: Int = 0,
        @SerializedName("warp_plus")
        var warpPlus: Boolean = false,
        @SerializedName("referral_count")
        var referralCount: Int = 0,
        @SerializedName("license")
        var license: String = "",
        @SerializedName("quota")
        var quota: Int = 0,
        @SerializedName("premium_data")
        var premiumData: Int = 0,
        @SerializedName("id")
        var id: String = "",
        @SerializedName("updated")
        var updated: String = ""
    )
}