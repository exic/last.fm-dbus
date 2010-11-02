/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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

#include "Fingerprinter.h"

#include <QDebug>
#include <QFile>
#include <QDateTime>

#include "fingerprint/OutCollectorDb.h"
//#include "fingerprint/DbKeyDefaults.h"

#include "utils/ioutils.h"
#include "utils/Sha256File.h"
#include "utils/Sha256.h"

using namespace fingerprint;

Fingerprinter::Fingerprinter () : QThread(), m_kgen(KeyGenerator::NUM_FRAMES_CLIENT)
{
    connect( this, SIGNAL( finished() ), SLOT( onThreadFinished() ) );
    reset();
}

// private slot:
void Fingerprinter::onThreadFinished()
{
    emit threadFinished( this );
}

void Fingerprinter::reset()
{
    m_data = QByteArray();
    m_track = TrackInfo();
    
    m_reset = true;
}

bool Fingerprinter::isFree()
{
    return !isRunning() && m_reset;
}

void Fingerprinter::start()
{
    if (isFree())
    {
        m_reset = false;
        QThread::start();
    }
    else
    {
        Q_ASSERT( !"Error: Fingerprinter-thread cannot start since it is not free. Always check isFree() first!" );
    }
}

void Fingerprinter::run()
{
    //qDebug() << "Fingerprinting thread started.";
    fingerprint(m_track.path());
}

QString Fingerprinter::sha256()
{
    QMutexLocker locker( &m_trackMutex );
    unsigned char hash[SHA256_HASH_SIZE];
    QString sha;
    Sha256File::getHash( m_track.path().toStdString(), hash );
    
    for (int i = 0; i < SHA256_HASH_SIZE; ++i) {
        QString hex = QString("%1").arg(uchar(hash[i]), 2, 16,
                                        QChar('0'));
        sha.append(hex);
    }
    
    return sha;
}

static void reverseBytes(QByteArray *buf)
{
    unsigned char *start = (unsigned char *)buf->data();
    unsigned char *end = start + buf->size() - 1;
    unsigned char c;
    while(start < end)
    {
        c = *start;
        *(start++) = *end;
        *(end--) = c;
    }
}

void Fingerprinter::fingerprint( QString filename )
{
    int time = QDateTime::currentDateTime().toTime_t();
    //qDebug() << "BEGIN: Fingerprinting";
    //qDebug() << "File " << filename;
    vector<unsigned int> keys;
    vector<GroupData> groupKeys;
    //KeyGenerator kgen( KeyGenerator::NUM_FRAMES_CLIENT );
    
    try
    {
        #ifdef _WIN32
        const std::wstring wstd_filename = filename.toStdWString();
        m_kgen.getKeys( wstd_filename, groupKeys );
        #else
        m_kgen.getKeys( std::string( QFile::encodeName ( filename ).constData() ), groupKeys );
        #endif
        //keys2GroupData( keys, groupKeys );
    }
    catch ( const string& str )
    {
        qDebug() << "Key Extraction error! " << str.c_str();
        QMutexLocker locker( &m_dataMutex );
        m_data.clear();
        return;
    }
    
    QMutexLocker locker( &m_dataMutex );
    m_data.clear();
    int numGroups = groupKeys.size();
    m_data.append( QByteArray( reinterpret_cast<const char*>(&numGroups), sizeof(unsigned int ) ) );
    m_data.append( QByteArray( reinterpret_cast<const char*>(&groupKeys[0]), sizeof(GroupData)*numGroups ) );
    
    //qDebug() << "Fingerprint size before compression " << m_data.size();
    m_data = qCompress ( m_data, 9 );
    //qDebug() << "Fingerprint size after compression" << m_data.size();
    
    //qDebug() << "END: Fingerprinting";
    time = QDateTime::currentDateTime().toTime_t() - time;
    //qDebug() << "Fingerprinting took " << time << " seconds";
}
