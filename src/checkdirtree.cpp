/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "checkdirtree.h"
#include "logger.h"

/******************************************************************************
    CheckDirModel::flags
******************************************************************************/
Qt::ItemFlags
CheckDirModel::flags(
    const QModelIndex& index) const
{
    return QDirModel::flags(index) | Qt::ItemIsUserCheckable;
}

/******************************************************************************
    CheckDirModel::data
******************************************************************************/
QVariant
CheckDirModel::data(
    const QModelIndex& index,
    int role) const
{
    if (role == Qt::CheckStateRole)
    {
        int id = index.internalId();
        return m_checkTable.contains(id) ? m_checkTable.value(id) : Qt::Checked;
    }
    else
    {
        return QDirModel::data(index, role);
    }
}    

/******************************************************************************
    CheckDirModel::setData
    
    Gets called when the user checks/unchecks through the GUI.
******************************************************************************/
bool
CheckDirModel::setData(
    const QModelIndex& index,
    const QVariant& value,
    int   role)
{
    if (role == Qt::CheckStateRole)
    {
        m_checkTable.insert(index.internalId(), (Qt::CheckState)value.toInt());

        emit dataChanged(index, index);
        emit dataChangedByUser(index);
        
        return true;
    }
    else
    {
        return QDirModel::setData(index, value, role);
    }
}    

/******************************************************************************
    CheckDirModel::setCheck
    
    Use for programmatically setting check state.
******************************************************************************/
void
CheckDirModel::setCheck(
    const QModelIndex& index,
    const QVariant& value)
{
    m_checkTable.insert(index.internalId(), (Qt::CheckState)value.toInt());
    emit dataChanged(index, index);
}    

/******************************************************************************
    CheckDirModel::getCheck
******************************************************************************/
Qt::CheckState
CheckDirModel::getCheck(
    const QModelIndex& index)
{
    return (Qt::CheckState)data(index, Qt::CheckStateRole).toInt();
}    

/******************************************************************************
    CheckDirTree::CheckDirTree
******************************************************************************/
CheckDirTree::CheckDirTree(
    QWidget* parent) :
        QTreeView(parent)
{
    m_dirModel.setFilter( QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks );
    setModel(&m_dirModel);
    setColumnHidden(1, true);
    setColumnHidden(2, true);
    setColumnHidden(3, true);
    //header()->hide();
    
    connect(&m_dirModel, SIGNAL(dataChangedByUser(const QModelIndex&)),
            this,        SLOT  (updateNode(const QModelIndex&)));

    connect(&m_dirModel, SIGNAL(dataChangedByUser(const QModelIndex&)),
            this,        SIGNAL(dataChanged()));

    connect(this, SIGNAL(collapsed(const QModelIndex&)),
            this, SLOT  (onCollapse(const QModelIndex&)));

    connect(this, SIGNAL(expanded(const QModelIndex&)),
            this, SLOT  (onExpand(const QModelIndex&)));

}

/******************************************************************************
    CheckDirTree::check
******************************************************************************/
void
CheckDirTree::checkPath(
    QString path,
    Qt::CheckState state)
{
    QModelIndex index = m_dirModel.index(path);
    m_dirModel.setCheck(index, state);
    updateNode(index);
}

/******************************************************************************
    CheckDirTree::setExclusions
******************************************************************************/
void
CheckDirTree::setExclusions(
    QStringList list)
{
    foreach(QString path, list)
    {
        checkPath(path, Qt::Unchecked);
    }
}

/******************************************************************************
    CheckDirTree::getExclusions
******************************************************************************/
QStringList
CheckDirTree::getExclusions()
{
    QStringList exclusions;
    QModelIndex root = rootIndex();

    getExclusionsForNode(root, exclusions);

    return exclusions;
}

/******************************************************************************
    CheckDirTree::getExclusionsForNode
******************************************************************************/
void
CheckDirTree::getExclusionsForNode(
    const QModelIndex& index,
    QStringList&       exclusions)
{
    // Look at first node
    // Is it checked?
    //  - move on to next node
    // Is it unchecked?
    //  - add to list
    //  - move to next node
    // Is it partially checked?
    //  - recurse

    int numChildren = m_dirModel.rowCount(index);
    for (int i = 0; i < numChildren; ++i)
    {
        QModelIndex kid = m_dirModel.index(i, 0, index);
        Qt::CheckState check = m_dirModel.getCheck(kid);
        if (check == Qt::Checked)
        {
            continue;
        }
        else if (check == Qt::Unchecked)
        {
            exclusions.append(m_dirModel.filePath(kid));
        }
        else if (check == Qt::PartiallyChecked)
        {
            getExclusionsForNode(kid, exclusions);
        }
        else
        {
            Q_ASSERT(false);
        }
    }
}    

/******************************************************************************
    CheckDirTree::onCollapse
******************************************************************************/
void
CheckDirTree::onCollapse(
    const QModelIndex& /*idx*/)
{

}

/******************************************************************************
    CheckDirTree::onExpand
******************************************************************************/
void
CheckDirTree::onExpand(
    const QModelIndex& idx)
{
    // If the node is partially checked, that means we have been below it
    // setting some stuff, so only fill down if we are unchecked.
    if (m_dirModel.getCheck(idx) != Qt::PartiallyChecked)
    {
        fillDown(idx);
    }
}

/******************************************************************************
    CheckDirTree::updateNode
    
    Updates the check state of visible nodes underneath the passed in node.    
******************************************************************************/
void
CheckDirTree::updateNode(
    const QModelIndex& idx)
{
    // Start by recursing down to the bottom and then work upwards
    fillDown(idx);
    updateParent(idx);
}

/******************************************************************************
    CheckDirTree::fillDown
    
    Takes a node index and propagates its state to all its child nodes.
******************************************************************************/
void
CheckDirTree::fillDown(
    const QModelIndex& parent)
{
    // Recursion stops when we reach a directory which has never been expanded
    // or one that has no children.
    if (!isExpanded(parent) ||
        !m_dirModel.hasChildren(parent))
    {
        return;
    }

    Qt::CheckState state = m_dirModel.getCheck(parent);
    int numChildren = m_dirModel.rowCount(parent);
    for (int i = 0; i < numChildren; ++i)
    {
        QModelIndex kid = m_dirModel.index(i, 0, parent);
        m_dirModel.setCheck(kid, state);
        fillDown(kid);
    }
}

/******************************************************************************
    CheckDirTree::updateParent
    
    Takes a node index and works out whether its parent is now checked,
    unchecked or partially checked. This is propagated up the tree.
******************************************************************************/
void
CheckDirTree::updateParent(
    const QModelIndex& index)
{
    QModelIndex parent = index.parent();
    
    if (!parent.isValid())
    {
        // We have reached the root
        return;
    }
    
    // Initialise overall state to state of first child
    QModelIndex kid = m_dirModel.index(0, 0, parent);
    Qt::CheckState overall = m_dirModel.getCheck(kid);

    int numChildren = m_dirModel.rowCount(parent);
    for (int i = 1; i <= numChildren; ++i)
    {
        kid = m_dirModel.index(i, 0, parent);
        Qt::CheckState state = m_dirModel.getCheck(kid);
        if (state != overall)
        {
            // If we ever come across a state different than the first child,
            // we are partially checked
            overall = Qt::PartiallyChecked;
            break;
        }
    }
    
    m_dirModel.setCheck(parent, overall);

    updateParent(parent);
}    
