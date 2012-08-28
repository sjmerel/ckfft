package com.crickettechnology.ckfft.test;

import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;

public class TestActivity extends Activity
{
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_test);

        test();
    }

    ////////////////////////////////////////

    private native void test();

    static 
    {
        System.loadLibrary("test");
    }
}
