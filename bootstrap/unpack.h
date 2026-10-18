#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

/* Write a temporary file; the caller authenticates it before publishing. */
static bool unpack_runtime(FILE *input,FILE *output,uint64_t expected) {
    unsigned char in[65536],out[65536];
    z_stream stream={0};uint64_t total=0;bool good=false;
    if(!input||!output||inflateInit(&stream)!=Z_OK)return false;
    for(;;) {
        if(!stream.avail_in) {
            size_t count=fread(in,1,sizeof(in),input);
            if(!count)break;
            stream.next_in=in;stream.avail_in=(uInt)count;
        }
        stream.next_out=out;stream.avail_out=sizeof(out);
        uInt before=stream.avail_in;
        int rc=inflate(&stream,Z_NO_FLUSH);
        size_t count=sizeof(out)-stream.avail_out;
        if(total>expected||count>expected-total)break;
        if(count&&fwrite(out,1,count,output)!=count)break;
        total+=count;
        if(rc==Z_STREAM_END) {
            good=total==expected&&!stream.avail_in&&fgetc(input)==EOF&&!ferror(input);
            break;
        }
        if(rc!=Z_OK||(!count&&before==stream.avail_in))break;
    }
    inflateEnd(&stream);
    return good&&!ferror(input)&&!ferror(output);
}
