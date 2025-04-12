package com.example.nativecpptest1;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;

public class MainActivity extends AppCompatActivity {

    protected CubeView graphicsView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        graphicsView = new CubeView(getApplication());
        setContentView(graphicsView);
        graphicsView.setOnTouchListener(new TouchListener(MainActivity.this) {
            public void onSwipeRight()
            {
                NativeLib.swipeRight();
            }
            public void onSwipeLeft()
            {
                NativeLib.swipeLeft();
            }
        });
    }

    @Override
    protected void onPause() {
        super.onPause();
        graphicsView.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        graphicsView.onResume();
    }

}