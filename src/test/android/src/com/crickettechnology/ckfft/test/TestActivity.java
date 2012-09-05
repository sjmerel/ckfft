package com.crickettechnology.ckfft.test;

import android.os.*;
import android.app.*;
import android.content.res.*;
import java.io.*;

public class TestActivity extends Activity
{
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_test);

        // copy asset files to external storage, so they're easier to access from native code
        copyAssetFiles();

        String externalStoragePath = getExternalFilesDir(null).getPath();
        test(externalStoragePath);
    }

    ////////////////////////////////////////

    private native void test(String externalStoragePath);

    private void copyAssetFiles()
    {
        AssetManager assets = getAssets();
        byte[] buf = new byte[10240];
        try
        {
            String[] assetFiles = assets.list("");
            for (String filename : assetFiles)
            {
                System.out.printf("copying %s\n", filename);
                try
                {
                    InputStream inStream = assets.open(filename);

                    String externalOutDirPath = getExternalFilesDir(null).getPath();
                    File externalOutDir = new File(externalOutDirPath);
                    externalOutDir.mkdirs();

                    FileOutputStream externalOutStream = new FileOutputStream(new File(externalOutDir, filename));

                    int bytesRead;
                    while ((bytesRead = inStream.read(buf)) != -1)
                    {
                        externalOutStream.write(buf, 0, bytesRead);
                    }

                    externalOutStream.close();

                    inStream.close();
                }
                catch (IOException ex) 
                {
                    System.out.println(ex);
                }
            }
        }
        catch (IOException ex) 
        {
            System.out.println(ex);
        }
    }


    static 
    {
        System.loadLibrary("test");
    }
}
