package org.hive13.shared;

import com.google.gwt.user.client.rpc.IsSerializable;

public class DisplayInfo implements IsSerializable {
	private int width;
	private int height;
	private String host;
	private int port;
	private String name;
	// First dimension will be 'width'; second, 'height'.
	private RGBColor[][] image;
	
	public int getWidth() {
		return width;
	}
	public void setDimensions(int width, int height) {
		this.width = width;
		this.height = height;
		image = new RGBColor[width][height];
	}
	public int getHeight() {
		return height;
	}
	public String getHost() {
		return host;
	}
	public void setHost(String host) {
		this.host = host;
	}
	public int getPort() {
		return port;
	}
	public void setPort(int port) {
		this.port = port;
	}
	public String getName() {
		return name;
	}
	public void setName(String name) {
		this.name = name;
	}
	public RGBColor[][] getImage() {
		return image;
	}

}
