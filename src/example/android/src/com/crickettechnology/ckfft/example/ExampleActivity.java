package com.crickettechnology.ckfft.example;

import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;

public class ExampleActivity extends Activity 
{
    @Override
    public void onCreate(Bundle savedInstanceState) 
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_example);
        main();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) 
    {
        getMenuInflater().inflate(R.menu.activity_example, menu);
        return true;
    }

    private native void main();

    static 
    {
        System.loadLibrary("example");
    }
}
