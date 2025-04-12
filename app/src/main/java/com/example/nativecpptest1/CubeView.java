package com.example.nativecpptest1;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.Log;

public class CubeView extends GLSurfaceView {
    public CubeView(Context context) {
        super(context);
        setEGLContextFactory(new ContextFactory());
        setEGLConfigChooser(new ConfigChooser());
        setRenderer(new CubeRenderer());
        Log.println(Log.DEBUG,"Tag","CubeView created");
    }
}
