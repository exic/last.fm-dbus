/***************************************************************************
 *   Copyright (C) 2007 by                                                 *
 *      Erik Jalevik, Last.fm Ltd <erik@last.fm>                           *
 *                                                                         *
 *   Based on RCStamp by Peter Chen:                                       *
 *   http://www.codeproject.com/tools/rcstamp.asp                          *
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

// This code isn't very safe as it uses loads of C string handling without
// much error checking. Only for use in internal build process.

//#include "stdafx.h"

#include <iostream>
#include <fstream>
#include <strstream>
#include <string>

using namespace std;

char const * strUsage =
" verupd command line:\r\n"
"\r\n"
"  verupd versionfile format files...\r\n"
"\r\n"
"  versionfile: header containing version define\r\n"
"\r\n"
"  format: specifies version number changes, e.g. *.*.33.+\r\n"
"          * : don't modify position\r\n"
"          + : increment position by one\r\n"
"          number : set position to this value\r\n"
"\r\n"
"  files: pairs of input and ouput files for which to replace version token with actual version number\r\n"
"\r\n";


// Constants

const int MAXLINELEN = 2048;


// Command Line Information

const char*    versionFile = NULL;   // name of version header file
const char*    format = NULL;        // format specifier
const char*    files[100];           // files to modify
int            numFiles = 0;

char           oldVersion[64];
char           newVersion[64];
char           versionTokenName[256];


bool ParseArgs(int argc, char* argv[])
{
    if ( argc < 3 )
    {
        cerr << "Wrong number of args" << endl;
        return false;
    }
        
    versionFile = argv[1];
    format = argv[2];    

    numFiles = argc - 3;
    if ( numFiles > 100 )
    {
        cerr << "Max 100 files" << endl;
        return false;
    }
    
    for ( int i = 3; i < argc; ++i )
    {
        files[i-3] = argv[i];
    }

    return true;
}

bool ReadOldVersion( const char* versionFile, char* oldVersion )
{
    ifstream is( versionFile );
    if (is.fail()) {
        cerr << "Cannot open " << versionFile << "\r\n";
        return false;
    }

    char line[MAXLINELEN];
    while (!is.eof())
    {
        is.getline(line, MAXLINELEN);
        if (is.bad())
        {
            cerr << "Error reading " << versionFile << "\r\n";
            return false;
        }

        char* token = strtok( line, " " );
        if ( token != NULL && strncmp( token, "#define", strlen( token ) ) == 0 )
        {
            token = strtok( NULL, " " );
            strncpy( versionTokenName, token, 255 );
            
            token = strtok( NULL, " " );
            if ( token == NULL )
                continue;
            
            // Remove the "s
            string nakedVersion( token );
            if ( token[0] == '\"' )
            {
                nakedVersion.erase( 0, 1 );
                nakedVersion.erase( nakedVersion.size() - 1, 1 );
            }            
            
            strncpy( oldVersion, nakedVersion.c_str(), 63 );
            
            break;
        }
    }

    is.close();
    
    return versionTokenName != 0 && oldVersion != 0;
}


bool CalcNewVersion(char const * oldVersion, char const * fmtstr, char * version)
{
    if (!fmtstr)
        fmtstr = format;

    char const * fmt[4];
    char * fmtDup = strdup(fmtstr);
    fmt[0] = strtok(fmtDup, " .,");
    fmt[1] = strtok(NULL,   " .,");
    fmt[2] = strtok(NULL,   " .,");
    fmt[3] = strtok(NULL,   " .,");

    if (fmt[3] == 0) {
        cerr << "Invalid Format\r\n";
        return false;
    }

    char * outHead = version;

    char * verDup = strdup(oldVersion);
    char * verStr = strtok(verDup, " ,.");

    *version = 0;

    for(int i=0; i<4; ++i) {
        int oldVersion = atoi(verStr);
        int newVersion = oldVersion;

        char c = fmt[i][0];

        if (strcmp(fmt[i], "*")==0)
            newVersion = oldVersion;
        else if (isdigit(c))
            newVersion = atoi(fmt[i]);
        else if (c=='+' || c=='-') {
            if (isdigit(fmt[i][1]))
                newVersion = oldVersion + atoi(fmt[i]);
            else
                newVersion = oldVersion + ((c=='+') ? 1 : -1);

        }

	sprintf(outHead, "%d", newVersion);
        outHead += strlen(outHead);

        if (i != 3) {
            strcpy(outHead, ".");
            outHead += 1;
            verStr = strtok(NULL, " ,.");
        }
    }
    free(fmtDup);
    free(verDup);
    return true;
}


bool ProcessFile(char const * fileIn, const char* fileOut, const char* token, char* newVersion)
{

    ifstream is(fileIn);
    if (is.fail()) {
        cerr << "Cannot open " << fileIn << "\r\n";
        return false;
    }

    char version[64] = { 0 };       // "final" version string
    strncpy( version, newVersion, 63 );

    // HACKETY HACK! If we're dealing with an rc file, we need commas, not dots.
    // Dreadful code too but I really can't be bothered to do this nicely now.
    if ( string( fileIn ).substr( strlen( fileIn ) - 3 ) == ".rc" )
    {
        for ( int i = 0; i < strlen( version ); ++i )
        {
            if ( version[i] == '.' )
                version[i] = ',';
        }
    }    

    string result;

    int tokenLen = strlen( token );
    char line[MAXLINELEN];

    while (!is.eof()) {
        is.getline(line, MAXLINELEN);
        if (is.bad()) {
            cerr << "Error reading " << fileIn << "\r\n";
            return false;
        }

        // Check for token in line
        char* location = strstr( line, token );
        while ( location != NULL )
        {
            char firstBit[MAXLINELEN];
            firstBit[0] = '\0';
            int firstBitLen = location - line;
            strncpy( firstBit, line, firstBitLen );
            firstBit[firstBitLen] = '\0';

            char secondBit[MAXLINELEN];
            secondBit[0] = '\0';
            int secondBitLen = strlen( line ) - tokenLen - (location - line);
            strncpy( secondBit,
                     location + tokenLen,
                     secondBitLen );
            secondBit[secondBitLen] = '\0';                     

            string substituted;
            substituted = string( firstBit ) + string( version ) + string( secondBit );
            strncpy( line, substituted.c_str(), MAXLINELEN );
                                    
            location = strstr( line, token );
        }
        
        result += line;
        result += "\n";
    }

    // Remove last line break we inserted
    result.erase( result.size() - 1, 1 );

    // re-write file
    is.close();

    ofstream os(fileOut);
    if (os.fail()) {
        cerr << "Cannot write " << fileOut << "\r\n";
        return false;
    }

    os.write( &result[0], result.size());

    if (os.fail()) {
        cerr << "Error writing " << fileOut << "\r\n";
        return false;
    }

    os.close();

    return true;
}



int main(int argc, char* argv[])
{
    if ( !ParseArgs(argc, argv) )
    {
        cerr << strUsage << endl;
        return 3;
    }

    // First, read version from file
    if ( !ReadOldVersion( versionFile, oldVersion ) )
        return 3;

    // Update version number according to format
    if ( !CalcNewVersion( oldVersion, format, newVersion ) )
        return 3;

    string oldv( oldVersion );
    string newv( newVersion );
    if ( oldv != newv )
    {
        if ( !ProcessFile( versionFile, versionFile, oldVersion, newVersion ) )
            return 3;
    }
    
    for ( int i = 0; i < numFiles; i += 2 )
    {
        if ( !ProcessFile( files[i], files[i+1], versionTokenName, newVersion ) )
            return 3;
    }

	return 0;
}
