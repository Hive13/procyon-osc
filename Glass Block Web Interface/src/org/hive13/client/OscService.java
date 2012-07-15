package org.hive13.client;

import com.google.gwt.user.client.rpc.RemoteService;
import com.google.gwt.user.client.rpc.RemoteServiceRelativePath;
import org.hive13.shared.DisplayInfo;
import org.hive13.shared.RGBColor;

@RemoteServiceRelativePath("osc")
public interface OscService extends RemoteService {
	
	// Get information about the display, like its size, its IP/port, and
	// what its name is.
	DisplayInfo getDisplayInfo();
	
	// Push out an update of the image (which the implementation stores)
	// to the remote display.
	void updateImage();
	
	// Toggle the value of a pixel and then push an update.
	void setPixel(int x, int y, RGBColor c);
	
	// Reset the image (i.e. make it all black) and push an update.
	void resetImage();
}
