package org.omnetpp.ned.editor.graph.edit;

import org.eclipse.gef.EditPart;
import org.eclipse.gef.EditPartFactory;
import org.omnetpp.ned.editor.graph.model.CompoundModuleNodeEx;
import org.omnetpp.ned.editor.graph.model.ConnectionNodeEx;
import org.omnetpp.ned.editor.graph.model.NedFileNodeEx;
import org.omnetpp.ned.editor.graph.model.SubmoduleNodeEx;

public class NedEditPartFactory implements EditPartFactory {

    public EditPart createEditPart(EditPart context, Object model) {
        EditPart child = null;

        if (model instanceof NedFileNodeEx) 
        	child = new NedFileDiagramEditPart();
        else if (model instanceof CompoundModuleNodeEx)
            child = new CompoundModuleEditPart();
        else if (model instanceof SubmoduleNodeEx)
            child = new SubmoduleEditPart();
        else if (model instanceof ConnectionNodeEx)
            child = new WireEditPart();
        else
        	System.out.println("Unknown model element: " + model.toString());
        child.setModel(model);
        return child;
    }

}
