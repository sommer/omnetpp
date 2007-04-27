package org.omnetpp.ned.model.interfaces;

import java.util.List;

import org.omnetpp.ned.model.INEDElement;
import org.omnetpp.ned.model.pojo.ExtendsNode;

/**
 * Objects that can extend other objects, ie they are derived objects.
 * @author rhornig
 */
public interface IHasAncestors extends INEDElement {

    /**
     * @return The base object's name (ONLY the first extends node name returned)
     */
    public String getFirstExtends();

    /**
     * @param ext The object's name that is extended
     */
    public void setFirstExtends(String ext);

    /**
     * @return The TTypeInfo object of the base object of this component. ie this method checks the base type
     *         of this element and looks up the typeinfo object to that. NOTE that this check only the
     *         FIRST extends node, so it returns the first extends child for ModuleInterface and ChannelInterface
     *         (Special handling needed for these types)
     */
    public INEDTypeInfo getFirstExtendsNEDTypeInfo();

    /**
     * @return The model element that represents the base object of this element
     *         NOTE that this check only the FIRST extends node, so it returns the first
     *         extends child for ModuleInterface and ChannelInterface
     *         (Special handling needed for these types)
     */
    public INEDElement getFirstExtendsRef();

    /**
     * @return All ned elements that used as base (usually only a single element,
     *         but can be more than that)
     */
    public List<ExtendsNode> getAllExtends();
}
