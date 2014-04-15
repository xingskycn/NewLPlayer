mv jni/player.h "jni/player.h.$(date --iso-8601=minutes)"
javah -o jni/player.h -classpath /home/misgood/android-sdk-linux/platforms/android-19/android.jar:./bin/classes com.misgood.newlplayer.Player
