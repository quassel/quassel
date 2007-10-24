#include <QObject>
#ifndef INT64_C
        #define INT64_C Q_INT64_C
#endif



#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>


int main (int,char**)
	{
        av_register_all();
        avcodec_init();
        avcodec_register_all();

	if(LIBAVCODEC_VERSION_INT>>16!=51)
		{
		qDebug("incompatible major version %i",LIBAVCODEC_VERSION_INT>>16);
		return 3;
		}
	return 0;
	}


