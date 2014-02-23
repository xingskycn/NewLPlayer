package com.misgood.newlplayer;

import java.util.Timer;
import java.util.TimerTask;

import android.content.Context;
import android.graphics.Point;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.net.Uri;
import android.os.AsyncTask;
import android.util.Log;
import android.view.Display;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.WindowManager;

public class Player {

	public interface OnPreparedListener {
		void onPrepared();
	}

	private static final String TAG = "Player";

	private String mPath;

	private Context mContext;

	private Surface mSurface;
	private SurfaceHolder mSurfaceHolder;

	// video info
	private double mFps;
	private int mVideoWidth;
	private int mVideoHeight;
	private int mScaledWidth;
	private int mScaledHeight;
	// audio info
	private int mSampleRate;

	private DecodeTask mDecodeTask;
	//private WriteAudioTask mWriteAudioTask;

	/*
	private Runnable mRunWriteAudio = new Runnable() {
		@Override
		public void run() {
			Log.d(TAG, "start WriteAudioTask");
			while(true) {
				naGetAudioData();
			}			
		}
	};
	*/
	
	private Timer mPlayerTimer;
	private TimerTask mDisplayTask;

	private int mAudioTrackBufferSize;
	private AudioTrack mAudioTrack;

	private OnPreparedListener mOnPreparedListener;

	private boolean isPrepared;
	private boolean isPlay;
	private boolean isDecodeComplete;

	class DisplayTask extends TimerTask {
		@Override
		public void run() {
			if(isPlay){
				if( naDisplay() == Error.ERROR_QUEUE_IS_EMPTY 
						&& isDecodeComplete 
						&& isPlay ) {
					// reach end of video, kill threads and release resource
					stop();
					naRelease();
				}
			}
		}
	}

	class DecodeTask extends AsyncTask<Object, Object, Object> {
		@Override
		protected void onPreExecute() {
			isDecodeComplete = false;
		}
		@Override
		protected Object doInBackground(Object... params) {
			Log.d(TAG, "start DecodeTask");
			naDecode();
			return null;
		}
		@Override
		protected void onPostExecute(Object result) {
			isDecodeComplete = true;
		}
		@Override
		protected void onCancelled(Object result) {
			naRelease();
		}
	}

	/*
	class WriteAudioTask extends AsyncTask<Object, Object, Object> {
		@Override
		protected Object doInBackground(Object... params) {
			Log.d(TAG, "start WriteAudioTask");
			while(true) {
				byte[] data = naGetAudioData();
				if(isDecodeComplete && data == null) {
					audioTrackWrite(null, 0, 0);
					break;
				}
				if(data != null) {
					Log.d(TAG, "audio data len: " + data.length);
					audioTrackWrite(data, 0, data.length);
				}
			}
			return null;
		}
	}
	*/

	/*
	class PrepareTask extends AsyncTask<Object, Object, Integer> {
		@Override
		protected Integer doInBackground(Object... arg0) {
			return naSetup(mPath, mSurface);
		}
		protected void onPostExecute(Integer result) {
			if(result != 0) {
				Log.e(TAG, "native setup fail");
				return;
			}
			// Video
			int[] videoRes = naGetVideoRes();
			if( videoRes == null ) {
				Log.e(TAG, "native get video resolution fail");
				return;
			}
			mVideoWidth = videoRes[0];
			mVideoHeight = videoRes[1];
			Log.i(TAG, "video width " + mVideoWidth + ", height " + mVideoHeight);
			mFps = naGetFps();
			if( mFps == 0 ) {
				Log.e(TAG, "cannot get fps");
				return;
			}
			Log.i(TAG, "fps: "+ mFps);
			resetVideoSize();

			// Audio
			int channelConfig = naGetChannels() >= 2 ? AudioFormat.CHANNEL_OUT_STEREO : AudioFormat.CHANNEL_OUT_MONO;
			mSampleRate = naGetSampleRate();
			Log.i(TAG, "sample rate: " + mSampleRate);
			mAudioTrackBufferSize = AudioTrack.getMinBufferSize(mSampleRate, channelConfig, AudioFormat.ENCODING_PCM_16BIT);
			mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, mSampleRate, channelConfig, AudioFormat.ENCODING_PCM_16BIT, mAudioTrackBufferSize, AudioTrack.MODE_STREAM);
			mAudioBuffer = new byte[mAudioTrackBufferSize];

			// prepare success
			isPrepared = true;
			if( mOnPreparedListener != null ) {
				mOnPreparedListener.onPrepared();
			}
		}
	}
	 */

	public Player(Context ctx) {
		this.mContext = ctx;
		this.isPrepared = false;
		this.isPlay = false;
		if( naInit() != 0 ) {
			Log.e(TAG, "native init fail");
			return;
		}
	}

	public void setVideoUri(Uri uri) {
		this.mPath = uri.toString();
	}

	public void setVideoPath(String path) {
		this.mPath = path;
	}

	public void resetVideoSize() {
		int windowWidth;
		int windowHeight;
		WindowManager wm = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE); 
		Display display = wm.getDefaultDisplay();
		Point size = new Point();
		display.getSize(size);
		windowWidth = size.x;
		windowHeight = size.y;
		Log.i(TAG, "window width " + windowWidth + ", height " + windowHeight);		
		float widthScaledRatio = windowWidth*1.0f/mVideoWidth;
		float heightScaledRatio = windowHeight*1.0f/mVideoHeight;
		if (widthScaledRatio > heightScaledRatio) {
			mScaledWidth = (int) (mVideoWidth*heightScaledRatio);
			mScaledHeight = windowHeight;
		} else {
			mScaledWidth = windowWidth;
			mScaledHeight = (int) (mVideoHeight*widthScaledRatio);
		}
		Log.i(TAG, "scaled width " + mScaledWidth + ", height " + mScaledHeight);
		mSurfaceHolder.setFixedSize(mScaledWidth, mScaledHeight);
	}

	public void prepare() {
		Log.d(TAG, "prepare");
		//new PrepareTask().execute();

		if(naSetup(mPath, mSurface) != 0) {
			Log.e(TAG, "native setup fail");
			return;
		}
		// Video
		int[] videoRes = naGetVideoRes();
		if( videoRes == null ) {
			Log.e(TAG, "native get video resolution fail");
			return;
		}
		mVideoWidth = videoRes[0];
		mVideoHeight = videoRes[1];
		Log.i(TAG, "video width " + mVideoWidth + ", height " + mVideoHeight);
		mFps = naGetFps();
		if( mFps == 0 ) {
			Log.e(TAG, "cannot get fps");
			return;
		}
		Log.i(TAG, "fps: "+ mFps);
		resetVideoSize();

		// Audio
		int channelConfig = naGetChannels() >= 2 ? AudioFormat.CHANNEL_OUT_STEREO : AudioFormat.CHANNEL_OUT_MONO;
		mSampleRate = naGetSampleRate();
		Log.i(TAG, "sample rate: " + mSampleRate);
		Log.i(TAG, "channel config: " + channelConfig);

		mAudioTrackBufferSize = AudioTrack.getMinBufferSize(mSampleRate, channelConfig, AudioFormat.ENCODING_PCM_16BIT);
		mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, mSampleRate, channelConfig, AudioFormat.ENCODING_PCM_16BIT, mAudioTrackBufferSize, AudioTrack.MODE_STREAM);

		// prepare success
		isPrepared = true;
		if( mOnPreparedListener != null ) {
			mOnPreparedListener.onPrepared();
		}
	}

	public void play() {
		Log.d(TAG, "play");
		if( isPrepared ) {
			isPlay = true;
			
			mDecodeTask = new DecodeTask();
			mDecodeTask.execute();	
			
			
			mPlayerTimer = new Timer();
			mDisplayTask = new DisplayTask();
			// testing
			mPlayerTimer.scheduleAtFixedRate(mDisplayTask, 0, (long) (1000/mFps));
			//mPlayerTimer.schedule(mDisplayTask, 0, (long) (1000/mFps));
			
		}
		else {
			Log.w(TAG, "video is not prepared");
		}
	}

	/*
	public void pause() {
		mDisplayTask.cancel();
		audioTrackPause();
	}

	public void resume() {
		mPlayerTimer.scheduleAtFixedRate(new DisplayTask(), 0, (long) (1000/mFps));
		audioTrackResume();
	}
	 */

	public void stop() {
		if(isPlay) {
			isPlay = false;
			if(!isDecodeComplete) {
				naStopDecode();
				mDecodeTask.cancel(false);
			}
			mPlayerTimer.cancel();
			audioTrackRelease();
		}
	}

	public void setSurfaceHolder(SurfaceHolder holder) {
		Log.d(TAG, "setSurfaceHolder");
		mSurfaceHolder = holder;
		mSurface = mSurfaceHolder.getSurface();
	}

	public void setOnPreparedListener(OnPreparedListener l) {
		this.mOnPreparedListener = l;
	}

	// call by native
	private void audioTrackWrite(byte[] audioData, int offsetInBytes, int sizeInBytes) {
		//Log.d(TAG, "audioTrackWrite");
		if (mAudioTrack != null) {
			audioTrackStart();
			int written;
			while (sizeInBytes > 0) {
				written = sizeInBytes > mAudioTrackBufferSize ? mAudioTrackBufferSize : sizeInBytes;
				mAudioTrack.write(audioData, offsetInBytes, written);
				sizeInBytes -= written;
				offsetInBytes += written;
			}
		}
		else {
			Log.e(TAG, "AudioTrack is null");
		}
	}

	private void audioTrackStart() {
		//Log.d(TAG, "audioTrackStart");
		if (mAudioTrack != null && 
				mAudioTrack.getState() == AudioTrack.STATE_INITIALIZED && 
				mAudioTrack.getPlayState() != AudioTrack.PLAYSTATE_PLAYING){
			Log.d(TAG, "start play audio");
			mAudioTrack.play();
		}
	}

	/*
	private void audioTrackPause() {
		if (mAudioTrack != null && mAudioTrack.getState() == AudioTrack.STATE_INITIALIZED)
			mAudioTrack.pause();
	}
	 */

	private void audioTrackRelease() {
		//Log.d(TAG, "audioTrackRelease");
		if (mAudioTrack != null) {
			if (mAudioTrack.getState() == AudioTrack.STATE_INITIALIZED)
				mAudioTrack.stop();
			mAudioTrack.release();
		}
		mAudioTrack = null;
	}

	private native int naInit(); 
	private native int naSetup(String pFileName, Surface pSurface);
	private native int[] naGetVideoRes();
	private native double naGetFps();
	private native int naGetSampleRate();
	private native int naGetChannels();
	private native void naDecode();
	private native byte[] naGetAudioData();
	private native int naDisplay();
	private native void naStopDecode();
	private native void naRelease();
	private native void naTest();

	static {
		System.loadLibrary("player");
	}
}
