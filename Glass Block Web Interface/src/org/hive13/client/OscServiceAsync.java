package org.hive13.client;

import com.google.gwt.user.client.rpc.AsyncCallback;
import org.hive13.shared.DisplayInfo;
import org.hive13.shared.RGBColor;

/**
 * The async counterpart of <code>OscService</code>.
 */
public interface OscServiceAsync {
	void getDisplayInfo(AsyncCallback<DisplayInfo> callback);

	// TODO: Figure out if I need to parametrize these when I cannot do it
	// with 'void'.
	void updateImage(AsyncCallback callback);
	
	void setPixel(int x, int y, RGBColor c, AsyncCallback callback);
	
	void resetImage(AsyncCallback callback);
}
