package org.hive13.server;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;

import javax.naming.Context;
import javax.naming.InitialContext;

import org.hive13.client.OscService;

import org.hive13.shared.DisplayInfo;
import org.hive13.shared.RGBColor;
import com.google.gwt.user.server.rpc.RemoteServiceServlet;

/**
 * The server side implementation of the RPC service.
 */
@SuppressWarnings("serial")
public class OscServiceImpl extends RemoteServiceServlet implements OscService {
	
	private DatagramSocket socket = null;
	private InetAddress remote = null;
	
	private DisplayInfo info = new DisplayInfo();
	
	public OscServiceImpl() throws UnknownHostException, SocketException {
		super();
		try {
			Context initCtx = new InitialContext();
			Context envCtx = (Context) initCtx.lookup("java:comp/env");
			String host = (String) envCtx.lookup("displayHost");
			int port = Integer.parseInt((String) envCtx.lookup("displayPort"));
			String name = (String) envCtx.lookup("displayName");
			int listenPort = Integer.parseInt((String) envCtx.lookup("listenPort"));
			
			int width = Integer.parseInt((String) envCtx.lookup("displayWidth"));
			int height = Integer.parseInt((String) envCtx.lookup("displayHeight"));
			
			if (host == null) {
			    host = "192.168.1.148";
			    port = 12000;
			    name = "Default Procyon board";
			    listenPort = 12001;
			    width = 7;
			    height = 8;
			}
			
            System.out.println("INIT: " + info.getHost() + ":" + info.getPort());
			
			info.setDimensions(width, height);
			info.setHost(host);
			info.setPort(port);
			info.setName(name);

			System.out.println("Opening socket...");
	
			socket = new DatagramSocket(listenPort);
			remote = InetAddress.getByName(info.getHost());
			// Start out with empty imagec
			RGBColor[][] image = info.getImage();
			for (int y = 0; y < info.getHeight(); ++y) {
				for (int x = 0; x < info.getWidth(); ++x) {
					image[x][y] = new RGBColor(0,0,0);
				}
			}
			
            System.out.println("Initialized display...");

			updateImage();
            System.out.println("updateImage() called...");
			
		} catch (Throwable t) {
			System.out.println("Exception thrown: " + t.getMessage());
		}
	}

	// Return information about the display - primarily, its size and where
	// it is.
	public DisplayInfo getDisplayInfo() {
		return info;
	}
	
	public void updateImage() {
		// TODO: Change this OSC path to something else!
		DatagramPacket p = buildPacket("/foo/blob", blobify());
		try {
			socket.send(p);
		} 
		catch(IOException e) {
			// TODO: Propagate this error better?
			System.out.println("IOException: " + e.getMessage());
		}		
	}
	
	public void setPixel(int x, int y, RGBColor c) {
		System.out.println("TOGGLING: " + x + "," + y);
		info.getImage()[x][y] = c;

		updateImage();
	}
	
	public void resetImage() {
		RGBColor[][] image = info.getImage();
		for(int n = 0; n < info.getHeight(); ++n) {
			for(int m = 0; m < info.getWidth(); ++m) {
				image[m][n].r = 0;
				image[m][n].g = 0;
				image[m][n].b = 0;
			}
		}
		
		updateImage();
	}
		
	private byte[] blobify() {
		final int width = info.getWidth();
		final int height = info.getHeight();
		byte msg[] = new byte[width * height * 3];
		for(int n = 0; n < height; ++n) {
			for(int m = 0; m < width; ++m) {
				RGBColor[][] image = info.getImage();
				int offset = (width * n + m) * 3;
				RGBColor c = image[m][n];
				msg[offset] = (byte) c.r; 
				msg[offset + 1] = (byte) c.g; 
				msg[offset + 2] = (byte) c.b; 
			}
		}
		return msg;
	}
	
	private DatagramPacket buildPacket(String addrPattern, byte[] blobArgument) {
		//Charset c = Charset.forName("UTF-8");
		byte [] addr = addrPattern.getBytes();
		int addrSize = addr.length + 1;
		// This pads to the nearest 4 bytes:
		addrSize = (addrSize + 3) & ~0x03;

		String typetag = ",b";
		byte [] tag = null;
		tag = typetag.getBytes();
		int tagSize = tag.length + 1;
		tagSize = (tagSize + 3) & ~0x03;

		int blobSize = blobArgument.length;
		blobSize = (blobSize + 3) & ~0x03;

		ByteBuffer blobSizeBuffer = ByteBuffer.allocate(4);
		blobSizeBuffer.putInt(blobSize);
		byte [] blobSizeBytes = blobSizeBuffer.array();

		// I don't understand this extra +4 at the end...
		int packetSize = addrSize + tagSize + 4 + blobSize + 4;
		byte [] packet = new byte[packetSize];
		int offset = 0;
		System.arraycopy(addr, 0, packet, offset, addr.length);
		offset += addrSize;
		System.arraycopy(tag, 0, packet, offset, tag.length);
		offset += tagSize;
		System.arraycopy(blobSizeBytes, 0, packet, offset, 4);
		offset += 4;
		System.arraycopy(blobArgument, 0, packet, offset, blobArgument.length);
		offset += blobSize;
		
		return new DatagramPacket(packet, packet.length, remote, info.getPort());
	}
	
}
