/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
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

/*! \class OutputInterface
    \brief Interface to Sound Output plugins.

    Interface to Output plugins which usually access the sound-card
    for audio output - or possibly even stream or write data elsewhere.
*/

#ifndef OUTPUT_INTERFACE_H
#define OUTPUT_INTERFACE_H

#include <QObject>


class OutputInterface : public QObject
{
    public:
        virtual ~OutputInterface() {}

        /** \brief Initialises the audio device.
          * \param sampelRate The sample-rate that will be used for audio data.
          * \param channels How many channels will be used. */
        virtual void initAudio( long sampleRate, int channels ) = 0;

        /** \brief Returns the current volume. (range: 0 to 1.0) */
        virtual float volume() = 0;

        /** \brief Pauses playback. */
        virtual void pause() = 0;

        /** \brief Resumes playback. */
        virtual void resume() = 0;

        /** \brief Returns true if there's any data in the output buffer. */
        virtual bool hasData() = 0;

        /** \brief Returns true if the buffer needs to be refilled. */
        virtual bool needsData() = 0;

        /** \brief Appends audio data for playback.
          * \param data The audio data in PCM. */
        virtual void processData( const QByteArray& data ) = 0;

        /** \brief Returns a list of supported Sound Systems. */
        virtual QStringList soundSystems() = 0;

        /** \brief Returns a list of available devices. */
        virtual QStringList devices() = 0;

        /** \brief Tell the output which sound device to use.
         *  \param n is the ordinal from the list returned by devices. */
        virtual void setDevice( int n ) = 0;

        /** \brief Sets the buffer capacity. */
        virtual void setBufferCapacity( int size ) = 0;

        /** \brief Returns the actual buffer size. */
        virtual int bufferSize() = 0;

        /** \brief Returns if the output plugin is currently active. */
        virtual bool isActive() = 0;

    public slots:
        /** \brief Clears the internal audio buffers. */
        virtual void clearBuffers() = 0;

        /** \brief Starts playback. */
        virtual void startPlayback() = 0;

        /** \brief Stops playback. */
        virtual void stopPlayback() = 0;

        /** \brief Changes the current volume. */
        virtual void setVolume( int volume ) = 0;

    signals:
        virtual void error( int error, const QString& reason ) = 0;
};

Q_DECLARE_INTERFACE( OutputInterface, "fm.last.Output/1.0" )

#endif
