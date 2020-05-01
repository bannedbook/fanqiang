package org.bannedbook.app.service;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.content.res.AssetManager;
import android.net.Uri;
import android.os.Environment;
import android.os.IBinder;
import android.util.Log;
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import android.content.Context;
import android.os.Build;

import com.github.shadowsocks.bg.ProxyService;
/*废弃，暂且没用*/
public class PolipoService extends Service {
    // A special ID assigned to this on-going notification.
    private static final int ONGOING_NOTIFICATION = 51888;
    private static Context mContext;
    static {
        System.loadLibrary("polipo");   //defaultConfig.ndk.moduleName
    }

    public static void startPolipoService(Context context ) {
        Log.e("startPolipoService","...");
        mContext=context;
        Intent intent = new Intent(context.getApplicationContext(), PolipoService.class);
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.N_MR1) {
            //context.startService(intent);
            context.startForegroundService(intent);
            //startForegroundService 必须5秒内调用startForeground createNotification，但createNotification后，关闭app重启时老崩溃，可能是http代理端口没有释放
            //所以暂且就用后台服务吧，也不创建通知了
        } else {
            context.startService(intent);
        }
    }

    public static void stopPolipoService() {
        Log.e("stopPolipoService","...");
        Intent intent = new Intent(mContext.getApplicationContext(), PolipoService.class);
        mContext.stopService(intent);
    }

    private String configFile;
    private Thread proxyThread;

    @Override
    public void onCreate() {
        Log.e("PolipoService","onCreate...");
        File rootDataDir = getFilesDir();
        final String toPath = rootDataDir.toString();
        String confFilename = "config.conf";
        configFile = toPath + "/"+confFilename;
        Log.e("toPath is: ", toPath);
        File file = new File(configFile);
        if (!file.exists()) copyAssets(confFilename, toPath);
        //file = new File(configFile);
        /*
        if (file.exists()) {
            int file_size = Integer.parseInt(String.valueOf(file.length()));
            Log.e("---", configFile + " exists, size is:" + file_size);
        }*/
        //new ProxyService().createNotification("JWproxy");

    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int id) {
        Log.e("PolipoService","onStartCommand...");
        new Thread(new Runnable() {
            @Override
            public void run() {
                NativeCall.execPolipo(configFile);
            }
        }).start();
        //Toast.makeText(this, "Service created!", Toast.LENGTH_LONG).show();
        //createNotification(); //共用ss startForeground通知
        return START_NOT_STICKY;
    }

    @Override
    public void onDestroy() {
        Log.e("PolipoService","onDestroy...");
        super.onDestroy();
        //new Executable().killPolipo();
        //stopForeground(true);
    }

    private void createNotification() {

        Intent notificationIntent = new Intent(this, mContext.getClass());
        //notificationIntent.setData(Uri.parse("http://127.0.0.1:8123"));
        //notificationIntent.setClassName(this,mContext.getClass().getName());
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0,notificationIntent, 0);

        Notification notification = new Notification.Builder(this).
                setContentTitle("JWProxy").
                setContentText("JWProxy - 禁闻代理正在运行。").
                setSmallIcon(android.R.drawable.ic_media_play).
                setOngoing(true).
                setContentIntent(pendingIntent).
                build();
        startForeground(ONGOING_NOTIFICATION, notification);
    }

    private void copyAssets(String configFile, String toPath) {
        AssetManager assetManager = getAssets();
        Log.e("---", "Copy file:  " + configFile);
        InputStream in = null;
        OutputStream out = null;
        try {
            in = assetManager.open(configFile);
            File outFile = new File(toPath, configFile);

            out = new FileOutputStream(outFile);
            copyFile(in, out);
            in.close();
            in = null;
            out.flush();
            out.close();
            out = null;
        } catch (IOException e) {
            Log.e("tag", "Failed to copy asset file: " + configFile, e);
        }

    }

    private void copyFile(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[1024];
        int read;
        while ((read = in.read(buffer)) != -1) {
            out.write(buffer, 0, read);
        }
    }

    public static String LOCAL_ASSETS_FILE="jwd.htm";
    public static String FILE_ON_SDCARD= Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)+File.separator+LOCAL_ASSETS_FILE;
    private void copyAssetsDataToSD(String strOutFileName) throws IOException
    {
        if(new File(FILE_ON_SDCARD).exists())
            return;

        InputStream myInput;
        OutputStream myOutput = new FileOutputStream(FILE_ON_SDCARD);//strOutFileName);
        myInput = this.getAssets().open(strOutFileName);
        byte[] buffer = new byte[1024];
        int length = myInput.read(buffer);
        while(length > 0)
        {
            myOutput.write(buffer, 0, length);
            length = myInput.read(buffer);
        }
        myOutput.flush();
        myInput.close();
        myOutput.close();
    }
    private void openAssetsHtmlFile() {
        File file=new File(FILE_ON_SDCARD);
        final Intent browserIntent = new Intent(Intent.ACTION_VIEW);
        browserIntent.setDataAndType(Uri.fromFile(file), "text/html");
        startActivity(Intent.createChooser(browserIntent, "首选Opera,Firefox浏览器,如果一种浏览器不行,可多换几种浏览器尝试..."));
    }
    synchronized boolean jwdOpen() {
        boolean isSucc=true;
        try {
            copyAssetsDataToSD(LOCAL_ASSETS_FILE);
        } catch (IOException e) {
            //Utils.showSnackbar(this, e.toString());
            isSucc=false;
        }

        if(isSucc){
            openAssetsHtmlFile();
        }
        return isSucc;
    }
}
