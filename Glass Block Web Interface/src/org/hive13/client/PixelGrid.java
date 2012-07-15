package org.hive13.client;

import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.user.client.Element;
import com.google.gwt.user.client.DOM;
import com.google.gwt.user.client.Event;
import com.google.gwt.user.client.rpc.AsyncCallback;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.Grid;
import com.google.gwt.user.client.ui.HTMLTable;
import com.google.gwt.user.client.ui.Panel;
import org.hive13.shared.DisplayInfo;
import org.hive13.shared.RGBColor;

public class PixelGrid extends Grid {
	
	private OscServiceAsync svc = null;
	private DisplayInfo info = null;
	
	public PixelGrid(DisplayInfo info, OscServiceAsync svc, Panel parent)
    {
		// Note that here, you give the number of rows first, then the
		// number of columns; this differs from how it is stored.
		super(info.getHeight(), info.getWidth());
		this.info = info;
		
        //DOM.setStyleAttribute(getElement(), "border", "1px dotted red");
        //DOM.setStyleAttribute(getElement(), "padding", "20px");
		sinkEvents(Event.ONMOUSEDOWN | Event.ONMOUSEUP | Event.ONMOUSEOUT);
		syncImage();
		setSize("300px", "300px");
		setCellSpacing(0);
		
		this.svc = svc;
		
		parent.add(this);
		parent.add(makeResetButton());
    }
	
	private Button makeResetButton() {
		final Button resetButton = new Button();
		resetButton.addClickHandler(new ClickHandler() {
			public void onClick(ClickEvent event) {
				System.out.println("clicked!");
				svc.resetImage(new AsyncCallback() {
					public void onFailure(Throwable caught) {
						//infoLabel.setText("Error communicating with server! " + caught.getMessage());
						System.out.println(caught.getMessage());
					}

					public void onSuccess(Object o) {
						resetImage();
					}
				});
			}
		});
		resetButton.setText("Clear");
		return resetButton;
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

		syncImage();
	}
	
	// Redraw the table in full from 'info'
	public void syncImage() {
		HTMLTable.CellFormatter formatter = getCellFormatter();
		for(int row = 0; row < info.getHeight(); ++row) {
			for(int col = 0; col < info.getWidth(); ++col) {
				RGBColor c = info.getImage()[col][row];
				formatter.getElement(row, col).setAttribute("BGCOLOR", c.getString());
			}
		}	
	}
	
    public void onBrowserEvent(Event event) {
        DOM.eventPreventDefault(event);
        Element td = getEventTargetCell(event);
        if (td == null) {
        	return;
        }
        Element tr = DOM.getParent(td);
        Element table = DOM.getParent(tr);
        final int row = DOM.getChildIndex(table, tr);
        final int column = DOM.getChildIndex(tr, td);
        System.out.println("Event at " + row + "," + column);
        switch (DOM.eventGetType(event))
        {
        case Event.ONMOUSEDOWN:
            /*
             * Get the mouse wheel information
             */
            //MouseWheelVelocity velocity = new MouseWheelVelocity(event);
            //count += velocity.getDeltaY();
            //setText("b");
            /*
             * Stop the event happening where it usually would. If we don't
             * do this, then the panel this widget is in would scroll if it
             * could
             */
            //DOM.eventPreventDefault(event);
        	//osc.togglePixel(row, column);
        	final RGBColor c = info.getImage()[column][row];
        	c.r = (255 - c.r);
        	c.g = (255 - c.g);
        	c.b = (255 - c.b);
        	svc.setPixel(column, row, c, new AsyncCallback() {
    			public void onFailure(Throwable caught) {
    				//info.setText("Error communicating with display! " + caught.getMessage());
    				System.out.println(caught.getMessage());
    			}

    			public void onSuccess(Object o) {
    				HTMLTable.CellFormatter formatter = getCellFormatter();
    				formatter.getElement(row, column).setAttribute("BGCOLOR", c.getString());
    				System.out.println("Setting to " + c.getString());
    				//info.getImage()[column][row] = c;
    			}
    		});
    		
        	System.out.println("Mouse down!");
            break;
        }
    }
}
