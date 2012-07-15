package org.hive13.client;

import org.hive13.shared.DisplayInfo;

import com.google.gwt.core.client.EntryPoint;
import com.google.gwt.core.client.GWT;
import com.google.gwt.user.client.rpc.AsyncCallback;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.RootPanel;

public class Glass_Block_Web_Interface implements EntryPoint {
	// Message displayed to user when server cannot be reached or returns an error.
	private static final String SERVER_ERROR = "An error occurred while "
			+ "attempting to contact the server. Please check your network "
			+ "connection and try again.";

	// Create a remote service proxy to talk to the server-side OSC service.
	private final OscServiceAsync oscService = GWT.create(OscService.class);
	
	// Entry point method.
	public void onModuleLoad() {
		//final TextBox info = new TextBox();
		final Label infoLabel = new Label();
		infoLabel.setText("Initializing...");

		final RootPanel rootPanel = RootPanel.get("nameFieldContainer");
		rootPanel.add(infoLabel);

		oscService.getDisplayInfo(new AsyncCallback<DisplayInfo>() {
			public void onFailure(Throwable caught) {
				infoLabel.setText("Error communicating with server! " + caught.getMessage());
				System.out.println(caught.getMessage());
			}

			public void onSuccess(DisplayInfo info) {
				infoLabel.setText("Server talking to '" + info.getName() + "', " + info.getWidth() + "x" + info.getHeight() + " at " + info.getHost() + ":" + info.getPort());
				PixelGrid grid = new PixelGrid(info, oscService, rootPanel);
			}
		});

	}
}
