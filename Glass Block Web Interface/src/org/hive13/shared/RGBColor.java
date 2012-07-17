package org.hive13.shared;

import com.google.gwt.user.client.rpc.IsSerializable;

public class RGBColor implements IsSerializable {
	// Just treat these like unsigned bytes.
	public int r, g, b;

	public RGBColor() {
		r = g = b = 0;
	}
	
	public RGBColor(int r, int g, int b) {
		this.r = r;
		this.g = g;
		this.b = b;
	}
	
	public String getString() {
		int rgb = r & 0xFF;
		rgb = (rgb << 8) + (g & 0xFF);
		rgb = (rgb << 8) + (b & 0xFF);
		
		return "#" + Integer.toHexString(rgb);
		//return "rgb(" + (int) r + " " + (int) g + " " + (int) b + ")";
	}
}
