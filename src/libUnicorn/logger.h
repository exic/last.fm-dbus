 /***************************************************************************
 *   Copyright 2005-2009 Last.fm Ltd.                                      *
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

#ifndef LOGGER_H
#define LOGGER_H

#ifdef WIN32
    #define COMMON_STD_STRING std::wstring
    #define COMMON_CHAR wchar_t
    #ifndef __PRETTY_FUNCTION__
        #define __PRETTY_FUNCTION__ __FUNCTION__
    #endif
#else
    #define COMMON_STD_STRING std::string
    #define COMMON_CHAR char
#endif

#include <fstream>
#include <sstream>
#ifdef WIN32
#include <windows.h> //for CRITICAL_SECTION
#endif

#include "UnicornDllExportMacro.h"

class UNICORN_DLLEXPORT Logger
{
    static Logger* instance;

    const COMMON_CHAR* filename;

public:
    enum Severity
    {
        Critical = 1,
        Warning,
        Info,
        Debug
    };
    
    const COMMON_CHAR* GetFilePath() const { return filename; }

    /** Sets the Logger instance to this, so only call once, and make it exist
      * as long as the application does */
    explicit Logger( const COMMON_CHAR* filename, Severity severity = Info );
    ~Logger();

    static Logger* the();

    void log( Severity level, const std::string& message, const char* function, int line );
    void log( Severity level, const std::wstring& message, const char* function, int line );
    
    /** plain write + flush, we suggest utf8 */
    void log( const char* message );

    static void truncate( const COMMON_CHAR* path );
    
private:
    const Severity mLevel;
#ifdef WIN32
    CRITICAL_SECTION mMutex;
#else
    pthread_mutex_t mMutex;
#endif
    std::ofstream mFileOut;
};


/** use these to log */
#define LOG( level, msg ) { \
    std::ostringstream ss; \
    ss << msg; \
    Logger* the = Logger::the(); \
    if (the) the->log( (Logger::Severity) level, ss.str(), __FUNCTION__, __LINE__ ); }
#define LOGL LOG
#define LOGW( level, msg ) { \
	std::wostringstream ss; \
	ss << msg; \
    Logger* the = Logger::the(); \
	if (the) the->log( (Logger::Severity) level, ss.str(), __FUNCTION__, __LINE__ ); }
#define LOGWL LOGW


#ifdef QT_CORE_LIB

#include <QDebug>
#include <QThread>
#include <QtGlobal>

/*************************************************************************/ /**
    Extra inserter to handle QStrings.
******************************************************************************/
inline std::ostream&
operator<<(std::ostream& os, const QString& qs)
{
    os << qs.toAscii().data();
    return os;
}

#ifndef QT_NO_DEBUG

/**
 * @author <max@last.fm> - mxcl
 * indents blocks, mostly used to mark function start and end automatically
 */

#include <QTime>
#include <QVariant>

class QDebugBlock
{
    mutable QString m_title; //because operator= requires a const parameter for some reason :(
    QTime m_time;

    static int &indents() { static int indent = 0; return indent; }

public:
    QDebugBlock( QString title ) : m_title( title )
    {
        // use data() to prevent quotes being output by QDebugStream
        debug() << "BEGIN:" << title.toLatin1().data();
        indents()++;

        m_time.start();
    }

    QDebugBlock( const QDebugBlock& that )
    {
        *this = that;
    }

    QDebugBlock &operator=( const QDebugBlock& block )
    {
        m_title = block.m_title;
        m_time = block.m_time;

        block.m_title = "";

        return *this;
    }

    ~QDebugBlock()
    {
        if (!m_title.isEmpty())
        {
            indents()--;

            // use data() to prevent quotes being output by QDebugStream
            debug() << "END:  " << m_title.toLatin1().data() << "[elapsed:" << m_time.elapsed() << "ms]";
        }
    }

    static QDebug debug()
    {
        QDebug d( QtDebugMsg );
        int i = indents() * 2;
        while (i--)
            d.space();

        return d;
    }


    /**
     * so you can do: Q_DEBUG_BLOCK << somestring;
     */
    QDebugBlock &operator<<( const QVariant& v )
    {
        QString const s = v.toString();
        if (!s.isEmpty())
            debug() << s;

        return *this;
    }
};

#define Q_DEBUG_BLOCK QDebugBlock mxcl_block = QDebugBlock( __PRETTY_FUNCTION__ )

#define qDebug() QDebugBlock::debug()


#else //Q_NO_DEBUG
    #include <QDateTime>

    #define Q_DEBUG_BLOCK qDebug()
    class QDebugBlock { public: QDebugBlock( QString ) {} };

    #define qDebug() qDebug() << QDateTime::currentDateTime().toUTC().toString( "yyMMdd hh:mm:ss" ) \
                              << '-' << QString("%1").arg( (int)QThread::currentThreadId(), 4 ) \
                              << '-' << __PRETTY_FUNCTION__ << '(' << __LINE__<< ") - L4\n  "

    #define qWarning() qDebug()
#endif

#endif //QT_CORE_LIB
#endif //header guard
