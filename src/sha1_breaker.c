// COMPILE : gcc sha1_breaker.c -o sha1_breaker -lssl -lcrypto -lm -fopenmp
// MAC OS X : clang sha1_breaker.c -o sha1_breaker -L/usr/local/opt/llvm/lib -I/usr/local/opt/llvm/include -lomp -Xpreprocessor -fopenmp -lm -L/usr/local/opt/openssl/lib -I/usr/local/opt/openssl/include -lcrypto -lssl
// RUN : OMP_NUM_THREADS=4 ./sha1_breaker


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <omp.h>
#include <math.h>  // pow()
#include <time.h>

#define SIZE_KEY 5
#define FIRST_CHAR 0x21
#define LAST_CHAR 0x7E
#define BUFFER_MAX 80


// copy an hex string sha1 into a correponding byte array
void sha1_string_2_bytes( char * sha, unsigned char * sha_bytes ){
    for ( int i = 0 ; i < 20 ; i++ )
        sscanf( &sha[2 * i], "%2hhx", &sha_bytes[i] ) ;
}

// compare two byte arrays for equality
int equals_arrays( unsigned char * a, unsigned char * b ){
    for ( int i = 0 ; i < 20 ; i++)
        if( a[i] != b[i] )
            return 0 ;
    return 1 ;
}

// main loop to break the hash
char * break_sha( unsigned char * challenge ){

    int loop_size = LAST_CHAR - FIRST_CHAR + 1 ;
    unsigned long int max_iter = powl( loop_size, SIZE_KEY - 1 ) ;

    unsigned char *result = ( unsigned char *) malloc( (SIZE_KEY+1) * sizeof(char) ) ;
    int num_thread = -1 ;
    int continuer = 1 ;

    #pragma omp parallel shared( result ) private( num_thread )
    {

        #pragma omp for
        for( int k = FIRST_CHAR ; k <= LAST_CHAR ; k++ ){

            if( continuer ){

                num_thread = omp_get_thread_num() ;
                unsigned char *tmp = ( unsigned char *) malloc( (SIZE_KEY+1) * sizeof(char) ) ;
                unsigned char *hash_bytes = ( unsigned char *) malloc( (SHA_DIGEST_LENGTH) * sizeof(char) ) ;

                //unsigned char hash_bytes[SHA_DIGEST_LENGTH] ;

                for( int i = 0 ; i < SIZE_KEY; i++ )
                    tmp[i] = FIRST_CHAR ;
                tmp[SIZE_KEY] = 0x00 ;

                tmp[0] = k ;
                printf("Thread %d start with : %s\n", num_thread, tmp ) ;

                for( unsigned long int j = 0 ; j < max_iter ; j++ ){

                    SHA1( tmp, SIZE_KEY, hash_bytes ) ;
                    if( equals_arrays( hash_bytes, challenge ) ){
                        #pragma atomic
                        strncpy(result, tmp, sizeof(tmp)) ;
                        continuer = 0 ;
                        break ;
                    }
                    else{
                        tmp[1]++ ;
                        for( int i = 1 ; i < SIZE_KEY - 1 ; i++ ){
                            if( tmp[i] == LAST_CHAR + 1 ){
                                tmp[i] = FIRST_CHAR ;
                                tmp[i+1]++ ;
                            }
                        }
                    }
                }// for
                free( tmp ) ;
                tmp = NULL ;
                free( hash_bytes ) ;
                hash_bytes = NULL ;
            }//if
        }// for parallel
    }//parallel
    return result  ;

}



int main( int argc, char** argv){

    if( argc != 2 ){
        printf( "<Usage> : %s sha1_in_hex_format\n", argv[0] ) ;
        return -1  ;
    }

    struct timeval t1 ;
    struct timeval t2 ;
    char buff[BUFFER_MAX] ;
    char challenge[41] = { 0 } ;
    unsigned char challenge_bytes[20] = {0} ;
    char * result ;


    strncpy(challenge, argv[1], 40) ;
    sha1_string_2_bytes( challenge, challenge_bytes ) ;

    gettimeofday(&t1, NULL);

    result = (char*) break_sha( challenge_bytes ) ;

    gettimeofday(&t2, NULL);
    printf( "time: %dmin %.3fs \n", (int)((t2.tv_sec-t1.tv_sec))/60, (double)((int)(t2.tv_sec-t1.tv_sec)%60)+((double)(t2.tv_usec-t1.tv_usec))/1000000 );

    printf( "Result : %s >> %s\n", argv[1], result) ;

    return 0  ;

}
