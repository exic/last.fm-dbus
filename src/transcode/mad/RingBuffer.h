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

#ifndef MOOSE_RING_BUFFER_H
#define MOOSE_RING_BUFFER_H

class QByteArray;
typedef unsigned int uint;

/** pretty raw (but fast) prolly-thread-safe RingBuffer implementation
  * TODO move to the lib under utils directory */
struct RingBuffer
{
    RingBuffer();

    char* m_buffer;
    int m_size;
    int m_read_index;
    int m_write_index;

    int size() const { return m_size - 1; }
    int free() const { return m_size - used() - 1; }
    int used() const
    {
        return (m_write_index >= m_read_index
             ? (m_write_index - m_read_index)
             : (m_size - (m_read_index - m_write_index) - 1));
    }

    void clear() { m_read_index = m_write_index = 0; }
    void write( const QByteArray& );
    void read( QByteArray& tofill );

private:
    /** NOTE this function may not be thread-safe, I think */
    void expandBy( uint amount );
};

#endif
