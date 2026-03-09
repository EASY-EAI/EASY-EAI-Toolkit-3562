#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


#include "system_opt.h"
#include "jpegCodec.h"

#define JPEG_FILE  "./Release/test.jpeg"
#define YUV_FILE  "./Release/srcData.yuv"

void *sendDatatoJpegDec(void *para)
{
    int *pipelineRuning = (int *)para;

    // 1. 打开文件
    int fd = open(JPEG_FILE, O_RDONLY);
    if (fd == -1) {
	    pthread_exit(NULL);
    }

    // 2. 获取文件大小
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
	    pthread_exit(NULL);
    }
    off_t file_size = st.st_size;

    // 3. 读取文件中的jpeg数据
    char *buffer = malloc(file_size);
    if(buffer){
        read(fd, buffer, file_size);
    }

    // 4. 循环多次发送jpeg数据
    int sendTimes = 1;  // 此处是jpeg数据的发送次数，可以被修改。
    while(*pipelineRuning){
        
        if(sendTimes <= 1) {    /* <=1，表示要发送最后一次，EndOfStream标志要置1 */
            JpegDec_pushData(buffer, (int)file_size, 1);
            break;
        }else{
            JpegDec_pushData(buffer, (int)file_size, 0);
            sendTimes--;
        }
        
        usleep(100*1000);
    }
    
    sleep(1);   //这个sleep不能删，要留点时间给pipeline销毁回收
    if(buffer){
        free(buffer);
    }
    
	pthread_exit(NULL);
}

int YUVData_output(void *yuvData, SrcDataDesc_t dataDesc)
{
    static int RecvDataTime = 1;
    printf("[%d] --- Decoded frame: %dx%d, format: %s, size: %d\n", RecvDataTime, dataDesc.width, dataDesc.height, dataDesc.fmt, dataDesc.dataSize);

    // 保存首次从jpeg解码出来的原始数据：YUV
    if(1 == RecvDataTime){
        FILE *fp_output = fopen(YUV_FILE, "w+b");
        if(fp_output){
            fwrite(yuvData, 1, dataDesc.dataSize, fp_output);
            fclose(fp_output);
        }
    }

    RecvDataTime++;
    return 0;
}




int main(int argc, char *argv[]) {
    int pipelineRuning = 0;
    // 1. 初始化Jpeg解码管道
    if(0 == JpegDec_init("jpegdec", YUVData_output)){
        pipelineRuning = 1;
    }

    // 2. 创建jpeg数据发送线程。启动jpeg解码管道后，进程会被阻塞，因此需要单独启动一个线程来向解码管道送入数据。
    pthread_t videoTid;
    CreateNormalThread(sendDatatoJpegDec, &pipelineRuning, &videoTid);

    // 3. 启动jpeg解码管道，进程会被阻塞在此处。
    JpegDec_start();

    // 4. 上面通过EOS表示释放管道的使用，管道被释放后，需要调用此接口释放所有管道资源。
    if(0 == JpegDec_unInit()){
        pipelineRuning = 0;
    }

    return 0;
}

