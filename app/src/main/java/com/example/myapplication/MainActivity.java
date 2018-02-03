package com.example.myapplication;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    // 加载函数库
    static {
      //  System.loadLibrary("ftpUpload");
        System.loadLibrary("ClientFtp" );
        //System.loadLibrary("OneThreadUpload");

    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
       /* Button OneUpButton= (Button) findViewById(R.id.upload_button);
        OneUpButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                FtpUploadFromJNI();
            }
        });*/
        Button ftpUpButton = (Button) findViewById(R.id.upload_button);
        ftpUpButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                FtpUploadFromJNI();
            }
        });
        Button OneDownButton=(Button) findViewById(R.id.download_button);
        OneDownButton.setOnClickListener(new View.OnClickListener(){

            @Override
            public void onClick(View view) {
                FtpDownloadFromJNI();
            }
        });


    }
    //本地方法，当前方法通过c/c++代码实现
    //public native String ftpUploadFromJNI()
    public native String FtpDownloadFromJNI();
    public native String FtpUploadFromJNI();
    //public native String OneThreadUploadFromJNI();
}
