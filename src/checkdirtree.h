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

#ifndef CHECKDIRTREE_H
#define CHECKDIRTREE_H

#include <QDirModel>
#include <QTreeView>


class CheckDirModel : public QDirModel
{
    Q_OBJECT

    public:

        CheckDirModel(
            QWidget* parent = NULL) : QDirModel(parent) { }

        virtual Qt::ItemFlags
        flags(
            const QModelIndex& index) const;

        virtual QVariant
        data(
            const QModelIndex & index,
            int role = Qt::DisplayRole ) const;

        virtual bool
        setData(
            const QModelIndex & index,
            const QVariant & value,
            int role = Qt::EditRole);

        void
        setCheck(
            const QModelIndex& index,
            const QVariant& value);

        Qt::CheckState
        getCheck(
            const QModelIndex& index);

    signals:

        void 
        dataChangedByUser(
            const QModelIndex & index);

    private:

        QHash<qint64, Qt::CheckState> m_checkTable;

};

class CheckDirTree : public QTreeView
{
    Q_OBJECT

    public:

        CheckDirTree(
            QWidget* parent);

        void
		checkPath( QString path,
               Qt::CheckState state );

        void
        setExclusions(
            QStringList list);

        QStringList
        getExclusions();

    signals:

        void
        dataChanged();

    private:

        CheckDirModel m_dirModel;
        QSet<qint64>  m_expandedSet;

        void
        fillDown(
            const QModelIndex& index);

        void
        updateParent(
            const QModelIndex& index);

        void
        getExclusionsForNode(
            const QModelIndex& index,
            QStringList&       exclusions);

    private slots:

        void
        onCollapse(
            const QModelIndex& idx);

        void
        onExpand(
            const QModelIndex& idx);

        void
        updateNode(
            const QModelIndex& idx);

};

#endif // CHECKDIRTREE_H
