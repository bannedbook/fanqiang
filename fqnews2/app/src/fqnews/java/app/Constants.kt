package app

import androidx.appcompat.app.AlertDialog
import java.util.Locale
import android.content.Context
import android.util.Log


object Constants {
    const val EMAIL_REPORT_ADDRESS: String = "banned.ebook@gmail.com"
    const val CRASH_REPORT_ADDRESS: String = "banned.ebook@gmail.com"
    var FEED_TAG = "FQnews"
    private val allFeeds = arrayOf(
        arrayOf("每日头条", "https://www.inoreader.com/stream/user/1005659457/tag/gb-topnews"),
        arrayOf("中共禁闻", "https://www.inoreader.com/stream/user/1005659457/tag/gb-bnews"),
        arrayOf("最新滚动", "https://www.inoreader.com/stream/user/1005659457/tag/gb-latest"),
        arrayOf("中国新闻", "https://www.inoreader.com/stream/user/1005659457/tag/gb-cnews"),
        //arrayOf("大陆新闻", "https://feeds.feedburner.com/bnews/djynews"),
        arrayOf("中国要闻", "https://www.inoreader.com/stream/user/1005659457/tag/gb-headline"),
        arrayOf("每日热点", "https://www.inoreader.com/stream/user/1005659457/tag/gb-hotnews"),
        arrayOf("国际新闻", "https://www.inoreader.com/stream/user/1005659457/tag/gb-worldnews"),
        arrayOf("禁闻评论", "https://www.inoreader.com/stream/user/1005659457/tag/gb-comments"),
        arrayOf("时事观察", "https://www.inoreader.com/stream/user/1005659457/tag/gb-ssgc"),
        arrayOf("中国人权", "https://www.inoreader.com/stream/user/1005659457/tag/gb-renquan"),
        arrayOf("香港新闻", "https://www.inoreader.com/stream/user/1005659457/tag/gb-hknews"),
        arrayOf("台湾新闻", "https://www.inoreader.com/stream/user/1005659457/tag/gb-twnews"),
        arrayOf("传统文化", "https://www.inoreader.com/stream/user/1005659457/tag/gb-tculture"),
        arrayOf("社会百态", "https://www.inoreader.com/stream/user/1005659457/tag/gb-baitai"),
        arrayOf("财经新闻", "https://www.inoreader.com/stream/user/1005659457/tag/gb-finance"),
        arrayOf("禁播视频", "https://www.inoreader.com/stream/user/1005659457/tag/gb-bannedvideo"),
        arrayOf("禁言博客", "https://www.inoreader.com/stream/user/1005659457/tag/gb-bblog"),
        arrayOf("维权上访", "https://www.inoreader.com/stream/user/1005659457/tag/gb-weiquan"),
        arrayOf("世界奥秘", "https://www.inoreader.com/stream/user/1005659457/tag/gb-aomi"),
        arrayOf("翻墙速递", "https://www.inoreader.com/stream/user/1005659457/tag/gb-fanqiang"),
        arrayOf("健康养生", "https://www.inoreader.com/stream/user/1005659457/tag/gb-health"),
        arrayOf("生活百科", "https://www.inoreader.com/stream/user/1005659457/tag/gb-lifebaike"),
        arrayOf("娱乐新闻", "https://www.inoreader.com/stream/user/1005659457/tag/gb-yule"),
        arrayOf("萌图囧视", "https://www.inoreader.com/stream/user/1005659457/tag/gb-funmedia"),
        arrayOf("其它禁闻", "https://www.inoreader.com/stream/user/1005659457/tag/others"),
        arrayOf("禁闻论坛", "https://feeds.feedburner.com/bannedbook/SqyN"),
        arrayOf("禁闻博客", "https://www.inoreader.com/stream/user/1005659457/tag/bblog"),
        //arrayOf("推特热点", "https://www.inoreader.com/stream/user/1005659457/tag/hot-twitter")
    )

    private val b5AllFeeds = arrayOf(
        arrayOf("台灣新聞", "https://www.inoreader.com/stream/user/1005659457/tag/b5-twnews"),
        arrayOf("香港新聞", "https://www.inoreader.com/stream/user/1005659457/tag/b5-hknews"),
        arrayOf("每日頭條", "https://www.inoreader.com/stream/user/1005659457/tag/b5-topnews"),
        arrayOf("中共禁聞", "https://www.inoreader.com/stream/user/1005659457/tag/b5-bnews"),
        arrayOf("中國新聞", "https://www.inoreader.com/stream/user/1005659457/tag/b5-cnews"),
        arrayOf("最新滾動", "https://www.inoreader.com/stream/user/1005659457/tag/b5-latest"),
        arrayOf("中國要聞", "https://www.inoreader.com/stream/user/1005659457/tag/b5-headline"),
        arrayOf("每日熱點", "https://feeds.feedburner.com/huaglad/b5cn"),
        arrayOf("國際新聞", "https://www.inoreader.com/stream/user/1005659457/tag/b5-worldnews"),
        arrayOf("禁聞評論", "https://www.inoreader.com/stream/user/1005659457/tag/b5-comments"),
        arrayOf("時事觀察", "https://www.inoreader.com/stream/user/1005659457/tag/b5-ssgc"),
        arrayOf("中國人權", "https://www.inoreader.com/stream/user/1005659457/tag/b5-renquan"),
        arrayOf("傳統文化", "https://www.inoreader.com/stream/user/1005659457/tag/b5-tculture"),
        arrayOf("社會百態", "https://www.inoreader.com/stream/user/1005659457/tag/b5-baitai"),
        arrayOf("財經新聞", "https://www.inoreader.com/stream/user/1005659457/tag/b5-finance"),
        arrayOf("禁播視頻", "https://www.inoreader.com/stream/user/1005659457/tag/b5-bannedvideo"),
        arrayOf("禁言博客", "https://www.inoreader.com/stream/user/1005659457/tag/b5-bblog"),
        arrayOf("維權上訪", "https://www.inoreader.com/stream/user/1005659457/tag/b5-weiquan"),
        arrayOf("世界奧秘", "https://www.inoreader.com/stream/user/1005659457/tag/b5-aomi"),
        arrayOf("翻牆速遞", "https://www.inoreader.com/stream/user/1005659457/tag/b5-fanqiang"),
        arrayOf("健康養生", "https://www.inoreader.com/stream/user/1005659457/tag/b5-health"),
        arrayOf("生活百科", "https://www.inoreader.com/stream/user/1005659457/tag/b5-lifebaike"),
        arrayOf("娛樂新聞", "https://www.inoreader.com/stream/user/1005659457/tag/b5-yule"),
        arrayOf("萌圖囧視", "https://www.inoreader.com/stream/user/1005659457/tag/b5-funmedia"),
        arrayOf("其它禁聞", "https://www.inoreader.com/stream/user/1005659457/tag/b5-others"),
        arrayOf("禁聞論壇", "https://www.inoreader.com/stream/user/1005659457/tag/b5-bbs"),
        arrayOf("禁聞博客", "https://www.inoreader.com/stream/user/1005659457/tag/bblog")
        //arrayOf("推特熱點", "https://www.inoreader.com/stream/user/1005659457/tag/hot-twitter")
    )
    var FEEDS = allFeeds
    //private val userCountry: String = Locale.getDefault().country
    //var isB5 =  "HK" == userCountry || "TW" == userCountry || "MO" == userCountry
    //var isCN =  "CN" == userCountry
fun initializeFeeds(context: Context, onFeedsInitialized: () -> Unit) {
        val languages = arrayOf("简体", "正體")
        val builder = AlertDialog.Builder(context)
        builder.setTitle("请选择语言")
    // 用户选择一个选项
        builder.setItems(languages) { _, which ->
            if (which == 1) {
                FEEDS = b5AllFeeds
                Log.e("debug-feed", "select B5 language")
            } else {
                //FEEDS = allFeeds
                Log.e("debug-feed", "select CN language")
            }
            onFeedsInitialized() // 在用户选择后调用回调
        }
        // 对话框被取消时调用
        builder.setOnCancelListener {
            //FEEDS = allFeeds
            Log.e("debug-feed", "Dialog was canceled")
            onFeedsInitialized() // 调用取消回调
        }
        builder.show()
    }
}
