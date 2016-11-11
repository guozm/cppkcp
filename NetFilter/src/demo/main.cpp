#include <stdio.h>
#include <memory.h>
#include "netfilter.h"
#include <iostream>

using namespace std;
#define ENCODING_BUFF_SIZE 10
#define ENCODING_RESULT_SIZE ENCODING_BUFF_SIZE * 50

int main(int argc, char **argv)
{
    char szSrcBuff[ENCODING_BUFF_SIZE];
    char szDstBuff[ENCODING_RESULT_SIZE];

    for(unsigned int i = 0; i < ENCODING_BUFF_SIZE; )
    {
        szSrcBuff[i] = 0x18; 
        szSrcBuff[i+1] = 0x18;
        szSrcBuff[i+2] = 0x18;
        szSrcBuff[i+3] = 0x18;
        szSrcBuff[i+4] = 0x18;
        i += 5;
    }

    NetFilter *pFilter = CreateNetFilter(NET_TPKT_FILTER);
    unsigned int nCurrentPos = 0;

    for(int k = 1; k <= ENCODING_BUFF_SIZE; k++)
    {
        CDataPackage package;
        pFilter->FilterWrite(szSrcBuff, k, package);
        if( nCurrentPos + package.GetDataLength() >= ENCODING_RESULT_SIZE )
        {
            cout << "Build test data has finished, nCurrentPos is " << nCurrentPos << "!\n";
            usleep(5000);
            break;
        }
        else
        {
            memcpy(szDstBuff+nCurrentPos, package.GetData(), package.GetDataLength());
            nCurrentPos += package.GetDataLength();
        }
    }

    for(unsigned int k = 1; k < nCurrentPos; k++)
    {
        printf("\nSize K is %d ----------------------------------------------------\n", k);

        // 每次传入5个字节
        for(unsigned int j = 0; j < nCurrentPos; )
        {
            if( pFilter->FilterRead(szDstBuff + j, (j + k >= nCurrentPos ? nCurrentPos - j : k)))
            {
                CDataPackage package;
                while( pFilter->GetPackage(package) )
                {
                    char *pValue = package.GetData();
                    unsigned int nLength = package.GetDataLength();
                    printf("\nRecive package size is %u\n .....................................................\n", nLength);

                    for(unsigned int i = 0; i < nLength; i++)
                    {
                        printf("%.2x", pValue[i]);
                        if( (i+1) % 4 ==0)
                        {
                            printf(" ");
                            if( (i+1) % 32 == 0)
                                printf("\n");
                        }
                    }
                    printf("\n-------------------------------------------------------------\n\n");
                }
            }
            j += k;
        }
   }
    delete pFilter;
    return 0;
}
