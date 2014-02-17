package com.misgood.newlplayer.demo;

import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;

import com.misgood.newlplayer.Player;
import com.misgood.newlplayer.R;

import android.net.Uri;
import android.os.Bundle;
import android.app.Activity;
import android.content.Intent;
import android.content.res.Configuration;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.Window;
import android.view.WindowManager;

public class MainActivity extends Activity implements SurfaceHolder.Callback, Player.OnPreparedListener {
	private static final String TAG = "MainActivity";
	
	private String videoPath = "rtsp://adelivery_s.emome.net:554/setn_gphone.sdp?dc=m20121026597105&msisdn=0988444669&ts=1351218248&at=1&mc=d2d58c";
	//private String videoPath = "/storage/sdcard0/Download/wp9.webm";
	private SurfaceView mSurfaceView;
	private SurfaceHolder mSurfaceHolder;
	private Player mPlayer;
	private boolean isAutoPlay = true;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.d(TAG, "onCreate");
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);
		setContentView(R.layout.activity_main);

		Intent intent = getIntent();
		String uriPath;

		if( intent != null ) {
			Uri uri = intent.getData();
			if( uri != null ) {
				uriPath = uri.getEncodedPath();
				try {
					uriPath = URLDecoder.decode(uriPath, "UTF-8");
				} catch (UnsupportedEncodingException e) {
					e.printStackTrace();
				}
				if( uriPath != null ){
					videoPath = uriPath;
					isAutoPlay = true;
				}
			}
		}

		Log.i(TAG, "video path: " + videoPath);

		mSurfaceView = (SurfaceView)findViewById(R.id.surfaceview);		
		mSurfaceHolder = mSurfaceView.getHolder();
		mSurfaceHolder.addCallback(this);

		mPlayer = new Player(this);
		mPlayer.setSurfaceHolder(mSurfaceHolder);
		mPlayer.setOnPreparedListener(this);
		Log.d(TAG, "onCreate done");
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		Log.d(TAG, "surfaceChanged: " + width + ":" + height);
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		Log.d(TAG, "surfaceCreated");
		if( isAutoPlay ) {
			mPlayer.setVideoPath(videoPath);
			mPlayer.prepare();
		}
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		Log.d(TAG, "surfaceDestroyed");
		mPlayer.stop();
	}

	@Override
	public void onPrepared() {
		if( isAutoPlay ) {
			mPlayer.play();
		}
	}

	@Override
	public void onConfigurationChanged(Configuration newConfig) {
		super.onConfigurationChanged(newConfig);
		if( newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE ) {
			mPlayer.resetVideoSize();
		}
		else if( newConfig.orientation == Configuration.ORIENTATION_PORTRAIT ) {
			mPlayer.resetVideoSize();
		}
	}
}
