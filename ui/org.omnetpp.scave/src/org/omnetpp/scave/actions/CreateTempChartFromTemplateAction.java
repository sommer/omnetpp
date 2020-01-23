/*--------------------------------------------------------------*

  Copyright (C) 2006-2015 OpenSim Ltd.

  This file is distributed WITHOUT ANY WARRANTY. See the file
  'License' for details on this and other legal matters.
*--------------------------------------------------------------*/

package org.omnetpp.scave.actions;

import org.eclipse.core.runtime.Assert;
import org.eclipse.jface.viewers.ISelection;
import org.omnetpp.scave.ScaveImages;
import org.omnetpp.scave.ScavePlugin;
import org.omnetpp.scave.charttemplates.ChartTemplate;
import org.omnetpp.scave.charttemplates.ChartTemplateRegistry;
import org.omnetpp.scave.editors.IDListSelection;
import org.omnetpp.scave.editors.ScaveEditor;
import org.omnetpp.scave.engine.IDList;
import org.omnetpp.scave.engine.ResultFileManager;
import org.omnetpp.scave.engine.ResultItemField;
import org.omnetpp.scave.engine.RunAttribute;
import org.omnetpp.scave.model.Chart;
import org.omnetpp.scave.model.Property;
import org.omnetpp.scave.model.ResultType;
import org.omnetpp.scave.model2.ResultSelectionFilterGenerator;
import org.omnetpp.scave.model2.ScaveModelUtil;

/**
 * Creates a temporary chart from the selection on the BrowseDataPage, and opens it.
 */
// TODO merge with CreateTempMatplotlibChartAction (common base?)
public class CreateTempChartFromTemplateAction extends AbstractScaveAction {

    ChartTemplate template;

    public CreateTempChartFromTemplateAction(ChartTemplate template) {
        this.template = template;
        setText("Plot using " + template.getName());
        setToolTipText("Plot using " + template.getName());


        String iconName = template.getToolbarIconPath();

        if (iconName == null || iconName.isEmpty())
            switch (template.getChartType()) {
                case BAR:        iconName = ScaveImages.IMG_ETOOL16_NEWBARCHART;        break;
                case HISTOGRAM:  iconName = ScaveImages.IMG_ETOOL16_NEWHISTOGRAMCHART;  break;
                case LINE:       iconName = ScaveImages.IMG_ETOOL16_NEWLINECHART;       break;
                case MATPLOTLIB: iconName = ScaveImages.IMG_ETOOL16_NEWMATPLOTLIBCHART; break;
            }
        setImageDescriptor(ScavePlugin.getImageDescriptor(iconName));
    }

    @Override
    protected void doRun(ScaveEditor editor, ISelection selection) {
        IDListSelection idListSelection = (IDListSelection)selection;
        ResultFileManager manager = idListSelection.getResultFileManager();
        IDList idList = idListSelection.getIDList();

        Chart chart = ScaveModelUtil.createChartFromTemplate(template);
        String[] filterFields = new String[] { RunAttribute.EXPERIMENT, RunAttribute.MEASUREMENT, RunAttribute.REPLICATION,
                ResultItemField.MODULE, ResultItemField.NAME };
        String viewFilter = editor.getBrowseDataPage().getActivePanel().getFilter().getFilterPattern();
        String filter = ResultFileManager.callWithReadLock(manager, () -> { return ResultSelectionFilterGenerator.getIDListAsFilterExpression(idList, filterFields, viewFilter, manager); });
        Property property = new Property("filter", filter);
        chart.addProperty(property);

        chart.setTemporary(true);
        editor.openPage(chart);
    }


    @Override
    protected boolean isApplicable(ScaveEditor editor, ISelection selection) {
        return selection instanceof IDListSelection && !selection.isEmpty() && ((IDListSelection)selection).getStatisticsCount() == 0;
    }
}
