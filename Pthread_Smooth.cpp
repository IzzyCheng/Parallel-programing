#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <pthread.h>
#include <semaphore.h>
#include "bmp.h"

using namespace std;

//define Smooth times
#define NSmooth 100

/*********************************************************/
/*variable declare                                       */
/*  bmpHeader    : BMPfile Header                        */
/*  bmpInfo      : BMPfile Info                          */
/*  **BMPSaveData: Store BmpData to be saved             */
/*  **BMPData    : temp BmpData to be Saved              */
/*********************************************************/
BMPHEADER bmpHeader;                        
BMPINFO bmpInfo;
RGBTRIPLE **BMPSaveData = NULL;                                               
int height = 0, width = 0;		//bmpInfo.Height and Width
int thread_count;			//global thread_count
pthread_mutex_t save_mutex;		//global mutex
int counter;				//global barrier counter
sem_t count_sem, barrier_sem;		//global barrier semaphore

/*********************************************************/
/*function declare                                       */
/*  readBMP    : read BMPfile and save in BMPSavaData    */
/*  saveBMP    : write BMPfile with data in BMPSaveData  */
/*  swap       : swap BMPData and BMPSaveData            */
/*  **alloc_memory : dynamic allocate a Y*X array        */
/*********************************************************/
int readBMP( char *fileName);        		//read file
int saveBMP( char *fileName);        		//save file
void swap(RGBTRIPLE *a, RGBTRIPLE *b);
RGBTRIPLE **alloc_memory(int Y, int X);
void Barrier();
void *Smooth(void *rank);
void *Hello(void *rank);

int main(int argc,char *argv[])
{
	/*********************************************************/
	/*variable declare                                       */
	/*  *infileName  : read file name                        */
	/*  *outfileName : write file name                       */
	/*  startwtime   : record startTime                      */
	/*  endwtime     : record endTime                        */
	/*********************************************************/
	char *infileName = "input.bmp";
	char *outfileName = "output1.bmp";

	//readfile
	if ( readBMP( infileName) )
		cout << "Read file successfully!!" << endl;
	else 
		cout << "Read file fails!!" << endl;

	//Get bmpInfo to be sent
	height = bmpInfo.biHeight;
	width = bmpInfo.biWidth;

	//malloc pthread_t memory
	thread_count = strtol(argv[1], NULL, 10);
	long thread;
	pthread_t* thread_handles;
	thread_handles = (pthread_t*)malloc(thread_count*sizeof(pthread_t));

	//init mutex for save BMPData into BMPSaveData
	pthread_mutex_init(&save_mutex, NULL);

	//init semaphore for barrier
	counter = 0;
	sem_init(&count_sem, 0, 1);
	sem_init(&barrier_sem, 0, 0);

	//create pthread for Smooth
	for (int i=0; i<thread_count; i++)
		pthread_create(&thread_handles[i], NULL, Smooth, (void*)i);

	for (int i=0; i<thread_count; i++)
		pthread_join(thread_handles[i], NULL);

	//Save BMP
	if (saveBMP(outfileName))
		cout << "Save file successfully!!" << endl;
	else
		cout << "Save file fails!!" << endl;

	//free memory
	free(BMPSaveData);
	pthread_mutex_destroy(&save_mutex);
	sem_destroy(&count_sem);
	sem_destroy(&barrier_sem);

	return 0;
}

void *Hello(void *rank) 
{
	long myrank = (long)rank;
	printf("Hello I'm thread %ld\n", myrank);
	Barrier();
	printf("Afer Barrier %ld\n", myrank);
	return NULL;
}
/*********************************************************/
void *Smooth(void *rank) {
	//thread local BMPData
	RGBTRIPLE **BMPData = NULL;
	BMPData = alloc_memory(height, width);

	long myrank = (long)rank;
	int myfirst = myrank*height/thread_count;
	int mylast = (myrank+1)*height/thread_count;

	//Start to Smooth Data
	for(int count = 0; count < NSmooth ; count ++)
	{
		//Smooth
		for(int i = myfirst; i<mylast; i++) {
			for(int j =0; j<width ; j++)
			{
				/*********************************************************/
				/*set the position of pixel around                       */
				/*********************************************************/
				int Top = i>0 ? i-1 : height-1;
				int Down = i<height-1 ? i+1 : 0;
				int Left = j>0 ? j-1 : width-1;
				int Right = j<width-1 ? j+1 : 0;
				/*********************************************************/
				/*computing pixel data and rounding                      */
				/*********************************************************/
				BMPData[i][j].rgbBlue =  (double) (BMPSaveData[i][j].rgbBlue+BMPSaveData[Top][j].rgbBlue+BMPSaveData[Down][j].rgbBlue+BMPSaveData[i][Left].rgbBlue+BMPSaveData[i][Right].rgbBlue)/5+0.5;
				BMPData[i][j].rgbGreen =  (double) (BMPSaveData[i][j].rgbGreen+BMPSaveData[Top][j].rgbGreen+BMPSaveData[Down][j].rgbGreen+BMPSaveData[i][Left].rgbGreen+BMPSaveData[i][Right].rgbGreen)/5+0.5;
				BMPData[i][j].rgbRed =  (double) (BMPSaveData[i][j].rgbRed+BMPSaveData[Top][j].rgbRed+BMPSaveData[Down][j].rgbRed+BMPSaveData[i][Left].rgbRed+BMPSaveData[i][Right].rgbRed)/5+0.5;
			}
		}
		Barrier();

		//critical section for saving global BMPSaveData
		pthread_mutex_lock(&save_mutex);
		for (int i = myrank*height/thread_count; i<(myrank+1)*height/thread_count; i++)
			for (int j=0; j<width; j++) {
				BMPSaveData[i][j].rgbBlue = BMPData[i][j].rgbBlue;
				BMPSaveData[i][j].rgbGreen = BMPData[i][j].rgbGreen;
				BMPSaveData[i][j].rgbRed = BMPData[i][j].rgbRed;
			}
		pthread_mutex_unlock(&save_mutex);
		Barrier();
	}

	free(BMPData);
	return NULL;
}
/*********************************************************/
/* read file                                             */
/*********************************************************/
int readBMP(char *fileName)
{
	//create read file object	
	ifstream bmpFile( fileName, ios::in | ios::binary );

	//file can't be opened
	if ( !bmpFile ){
		cout << "It can't open file!!" << endl;
		return 0;
	}

	//read BMPHeader data
	bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );

	//determine BMPfile or not
	if( bmpHeader.bfType != 0x4d42 ){
		cout << "This file is not .BMP!!" << endl ;
		return 0;
	}

	//read BMPInfo
	bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );

	//determine if bit depth is 24 bits
	if ( bmpInfo.biBitCount != 24 ){
		cout << "The file is not 24 bits!!" << endl;
		return 0;
	}

	//fix the width to be multiple of 4
	while( bmpInfo.biWidth % 4 != 0 )
		bmpInfo.biWidth++;

	//dynamically allocate memory
	BMPSaveData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);

	//read pixel data
	//for(int i = 0; i < bmpInfo.biHeight; i++)
	//	bmpFile.read( (char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE));
	bmpFile.read( (char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight);

	//close file
	bmpFile.close();

	return 1;

}
/*********************************************************/
/* sava BMPData                                              */
/*********************************************************/
int saveBMP( char *fileName)
{
	//determine BMPfile or not
	if( bmpHeader.bfType != 0x4d42 ){
		cout << "This file is not .BMP!!" << endl ;
		return 0;
	}

	//create weite file object
	ofstream newFile( fileName,  ios:: out | ios::binary );

	//file can't be created
	if ( !newFile ){
		cout << "The File can't create!!" << endl;
		return 0;
	}

	//write BMPHeader data
	newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );

	//write BMPData
	newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );

	//write pixel data
	//for( int i = 0; i < bmpInfo.biHeight; i++ )
	//        newFile.write( ( char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE) );
	newFile.write( ( char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight );

	//write file
	newFile.close();

	return 1;

}


/*********************************************************/
/* allocate memory : return Y*X array                    */
/*********************************************************/
RGBTRIPLE **alloc_memory(int Y, int X )
{        
	//create height Y pointer array
	RGBTRIPLE **temp = new RGBTRIPLE *[ Y ];
	RGBTRIPLE *temp2 = new RGBTRIPLE [ Y * X ];
	memset( temp, 0, sizeof( RGBTRIPLE ) * Y);
	memset( temp2, 0, sizeof( RGBTRIPLE ) * Y * X );

	//declare a length of X array to pointer array
	for( int i = 0; i < Y; i++){
		temp[ i ] = &temp2[i*X];
	}

	return temp;

}
/*********************************************************/
/* swap two pointer                                      */
/*********************************************************/
void swap(RGBTRIPLE *a, RGBTRIPLE *b)
{
	RGBTRIPLE *temp;
	temp = a;
	a = b;
	b = temp;
}
/*********************************************************/
void Barrier()
{
	sem_wait(&count_sem);
	if (counter == thread_count-1) {
		counter = 0;
		sem_post(&count_sem);
		for (int i=0; i<thread_count-1; i++)
			sem_post(&barrier_sem);
	} else {
		counter++;
		sem_post(&count_sem);
		sem_wait(&barrier_sem);
	}
	return;
}
/*********************************************************/
