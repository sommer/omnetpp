package org.omnetpp.ned.editor.graph.edit;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.XYLayout;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.gef.AutoexposeHelper;
import org.eclipse.gef.CompoundSnapToHelper;
import org.eclipse.gef.EditPolicy;
import org.eclipse.gef.ExposeHelper;
import org.eclipse.gef.GraphicalEditPart;
import org.eclipse.gef.MouseWheelHelper;
import org.eclipse.gef.SnapToGeometry;
import org.eclipse.gef.SnapToGrid;
import org.eclipse.gef.SnapToGuides;
import org.eclipse.gef.SnapToHelper;
import org.eclipse.gef.editparts.ViewportAutoexposeHelper;
import org.eclipse.gef.editparts.ViewportExposeHelper;
import org.eclipse.gef.editparts.ViewportMouseWheelHelper;
import org.eclipse.gef.editpolicies.SnapFeedbackPolicy;
import org.eclipse.gef.rulers.RulerProvider;
import org.eclipse.swt.graphics.Image;
import org.omnetpp.common.color.ColorFactory;
import org.omnetpp.common.image.ImageFactory;
import org.omnetpp.ned.editor.graph.edit.policies.CompoundModuleLayoutEditPolicy;
import org.omnetpp.ned.editor.graph.figures.CompoundModuleFigure;
import org.omnetpp.ned.editor.graph.figures.properties.DisplayCalloutSupport;
import org.omnetpp.ned.editor.graph.figures.properties.DisplayInfoTextSupport;
import org.omnetpp.ned.editor.graph.figures.properties.DisplayNameSupport;
import org.omnetpp.ned.editor.graph.figures.properties.DisplayQueueSupport;
import org.omnetpp.ned.editor.graph.figures.properties.DisplayRangeSupport;
import org.omnetpp.ned.editor.graph.figures.properties.DisplayShapeSupport;
import org.omnetpp.ned.editor.graph.figures.properties.DisplayTooltipSupport;
import org.omnetpp.ned2.model.CompoundModuleNodeEx;
import org.omnetpp.ned2.model.DisplayString;
import org.omnetpp.ned2.model.INedModule;

public class CompoundModuleEditPart extends NedNodeEditPart {


    @Override
    protected void createEditPolicies() {
        super.createEditPolicies();
        installEditPolicy(EditPolicy.LAYOUT_ROLE, new CompoundModuleLayoutEditPolicy((XYLayout) getContentPane()
                .getLayoutManager()));
        installEditPolicy("Snap Feedback", new SnapFeedbackPolicy()); //$NON-NLS-1$
    }

    /**
     * Creates a new Module Figure and returns it.
     * 
     * @return Figure representing the module.
     */
    @Override
    protected IFigure createFigure() {
        return new CompoundModuleFigure();
    }

    /**
     * Returns the Figure of this as a ModuleFigure.
     * 
     * @return ModuleFigure of this.
     */
    protected CompoundModuleFigure getModuleFigure() {
        return (CompoundModuleFigure) getFigure();
    }

    @Override
    public IFigure getContentPane() {
        return getModuleFigure().getContentsPane();
    }

    @Override
    public Object getAdapter(Class key) {
        
        if (key == AutoexposeHelper.class) return new ViewportAutoexposeHelper(this);
        
        if (key == ExposeHelper.class) return new ViewportExposeHelper(this);

        if (key == MouseWheelHelper.class) return new ViewportMouseWheelHelper(this);

        // snap to grig/guide adaptor
        if (key == SnapToHelper.class) {
            List snapStrategies = new ArrayList();
            Boolean val = (Boolean) getViewer().getProperty(RulerProvider.PROPERTY_RULER_VISIBILITY);
            if (val != null && val.booleanValue()) snapStrategies.add(new SnapToGuides(this));
            val = (Boolean) getViewer().getProperty(SnapToGeometry.PROPERTY_SNAP_ENABLED);
            if (val != null && val.booleanValue()) snapStrategies.add(new SnapToGeometry(this));
            val = (Boolean) getViewer().getProperty(SnapToGrid.PROPERTY_GRID_ENABLED);
            if (val != null && val.booleanValue()) snapStrategies.add(new SnapToGrid(this));

            if (snapStrategies.size() == 0) return null;
            if (snapStrategies.size() == 1) return snapStrategies.get(0);

            SnapToHelper ss[] = new SnapToHelper[snapStrategies.size()];
            for (int i = 0; i < snapStrategies.size(); i++)
                ss[i] = (SnapToHelper) snapStrategies.get(i);
            return new CompoundSnapToHelper(ss);
        }
        
        return super.getAdapter(key);
    }

    @Override
    protected List getModelChildren() {
    	return ((CompoundModuleNodeEx)getNEDModel()).getModelChildren();
    }
    /**
     * Updates the visual aspect of this.
     */
    // FIXME most of this stuff should go to SubmoduleEditPart.refreshVisuls()
    @Override
    protected void refreshVisuals() {
        
        // define the properties that determine the visual appearence
        
    	INedModule model = (INedModule)getNEDModel();
    	
        // parse a dispaly string, so it's easier to get values from it.
        DisplayString dps = model.getDisplayString();
        
        // setup the figure's properties

        Integer x = dps.getAsIntDef(DisplayString.Prop.X, 0);
        Integer y = dps.getAsIntDef(DisplayString.Prop.Y, 0);
        Integer w = dps.getAsIntDef(DisplayString.Prop.WIDTH, -1);
        Integer h = dps.getAsIntDef(DisplayString.Prop.HEIGHT, -1);
        // set the location and size using the models helper methods
        Rectangle constraint = new Rectangle(x, y, w, h);
        ((GraphicalEditPart) getParent()).setLayoutConstraint(this, getFigure(), constraint);
        // check if the figure supports the name decoration
        if(getNedFigure() instanceof DisplayNameSupport)
            ((DisplayNameSupport)getNedFigure()).setName(model.getName());
        // tooltip support
        if(getNedFigure() instanceof DisplayTooltipSupport)
            ((DisplayTooltipSupport)getNedFigure()).setTooltipText(
                    dps.getAsStringDef(DisplayString.Prop.TOOLTIP));
        // shape support
        if(getNedFigure() instanceof DisplayShapeSupport) {
            String imgSize = dps.getAsStringDef(DisplayString.Prop.IMAGESIZE);
            Image img = ImageFactory.getImage(
                    dps.getAsStringDef(DisplayString.Prop.IMAGE), 
                    imgSize,
                    ColorFactory.asRGB(dps.getAsStringDef(DisplayString.Prop.IMAGECOLOR)),
                    dps.getAsIntDef(DisplayString.Prop.IMAGECOLORPCT,0));
            
            // set the figure properties
            ((DisplayShapeSupport)getNedFigure()).setShape(img, 
                    dps.getAsStringDef(DisplayString.Prop.SHAPE), 
                    dps.getAsIntDef(DisplayString.Prop.WIDTH, -1), 
                    dps.getAsIntDef(DisplayString.Prop.HEIGHT, -1),
                    ColorFactory.asColor(dps.getAsStringDef(DisplayString.Prop.FILLCOL)),
                    ColorFactory.asColor(dps.getAsStringDef(DisplayString.Prop.BORDERCOL)),
                    dps.getAsIntDef(DisplayString.Prop.BORDERWIDTH, -1));
            // set the decoration image properties
            ((DisplayShapeSupport)getNedFigure()).setImageDecoration(
                    ImageFactory.getImage(
                            dps.getAsStringDef(DisplayString.Prop.OVIMAGE),
                            null,
                            ColorFactory.asRGB(dps.getAsStringDef(DisplayString.Prop.OVIMAGECOLOR)),
                            dps.getAsIntDef(DisplayString.Prop.OVIMAGECOLORPCT,0)));

        }
        
    }
}
