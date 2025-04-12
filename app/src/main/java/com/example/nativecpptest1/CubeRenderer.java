package com.example.nativecpptest1;

import android.opengl.GLSurfaceView;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

class CubeRenderer implements GLSurfaceView.Renderer
{
    public void onDrawFrame(GL10 gl)
    {
        NativeLib.step();
    }
    public void onSurfaceChanged(GL10 gl, int width, int height)
    {
        NativeLib.init(width,height);
    }
    public void onSurfaceCreated(GL10 gl, EGLConfig config)
    {
    }
}