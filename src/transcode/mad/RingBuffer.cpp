/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      John Stamp, <jstamp@users.sourceforge.net>                         *
 *      Max Howell, Last.fm Ltd <max@last.fm>                              *
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

#include "RingBuffer.h"

#include <logger.h>
#include <QByteArray>
#include <stdlib.h>
#include <QtGlobal>


RingBuffer::RingBuffer() : m_buffer( 0 ),
                   m_size( 1 ),
                   m_read_index( 0 ),
                   m_write_index( 0 )
{
    // A better start value? They'll grow as needed anyway.
    m_buffer = (char*) malloc( m_size );
}


void
RingBuffer::write( const QByteArray& buffer )
{
    Q_DEBUG_BLOCK << "towrite:" << buffer.size();
    qDebug() << "used" << used();
    qDebug() << "free" << free();
    qDebug() << "size" << size();

    int length = buffer.size();
    const char* src = buffer.data();
    const uint free = this->free();

    if (length > (int)free)
        expandBy( length - free + 1 );

    for (int cnt; length > 0; )
    {
        cnt = qMin( length, m_size - m_write_index );
        memcpy( m_buffer + m_write_index, src, cnt );
        m_write_index = (m_write_index + cnt) % m_size;
        length -= cnt;
        src += cnt;
    }
}


void
RingBuffer::read( QByteArray& tofill )
{
    Q_DEBUG_BLOCK << "toread:" << tofill.size();
    qDebug() << "used" << used();
    qDebug() << "free" << free();
    qDebug() << "size" << size();

    if (tofill.size() > used())
        tofill.resize( used() );

    int length = tofill.size();
    char* out = tofill.data();

    while (length > 0)
    {
        int cnt = qMin( length, m_size - m_read_index );
        memcpy( out, m_buffer + m_read_index, cnt );
        m_read_index = (m_read_index + cnt) % m_size;
        length -= cnt;
        out += cnt;
    }
}


void
RingBuffer::expandBy( uint amount )
{
    //Q_DEBUG_BLOCK << amount;

    char* tmp = (char*)realloc( m_buffer, sizeof(char) * ( m_size + amount ) );

    if (!tmp) {
        Q_ASSERT( !"Could not reallocate buffer!" );
        return;
    }

    m_buffer = tmp;

    // Don't screw up the read order or ugly noises will ensue
    if (m_read_index > m_write_index)
    {
        memmove( m_buffer + m_read_index + amount,
                 m_buffer + m_read_index,
                 m_size - m_read_index );
        m_read_index += amount;
    }

    m_size += amount;

    //qDebug() << m_size;
}
