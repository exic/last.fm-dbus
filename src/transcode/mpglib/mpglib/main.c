#include "stdlib.h"
#include "unistd.h"
#include "mpg123.h"
#include "mpglib.h"

char buf[16384];
struct mpstr mp;

int main(int argc,char **argv)
{
	int size;
	char out[8192];
	char swapped;
	int len,ret;
	int i;
        FILE *fp;	

	InitMP3(&mp);

	fp = fopen( "lala.wav", "w" );

	printf( "starting\n" );

	while(1) {
		len = read(0,buf,16384);
		if(len <= 0)
			break;

		ret = decodeMP3(&mp,buf,len,out,8192,&size);

		printf( "samplerate: %d - channels: %d\n", freqs[mp.fr.sampling_frequency], mp.fr.stereo );
		
		while(ret == MP3_OK) {
			printf( "written: %d\n", size );
			
			for ( i = 0; i < ( size / 2 ); i++ )
			{
				fputc( out[i*2 + 1], fp );
				fputc( out[i*2], fp );
			}
			
			ret = decodeMP3(&mp,NULL,0,out,8192,&size);
		}
	}

	fclose( fp );
	return 0;
}

