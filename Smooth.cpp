#include <mpi.h>
#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "bmp.h"

using namespace std;

//define Smooth times
#define NSmooth 1000

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
RGBTRIPLE **BMPData = NULL;                                                   

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
RGBTRIPLE **alloc_memory( int Y, int X );	//allocate memory

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
	char *outfileName = "output2.bmp";
	double startwtime = 0.0, endwtime=0;
	int numprocs, myid;
	int height = 0, width = 0;

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	if (myid == 0)
	{
		//readfile
		if ( readBMP( infileName) )
			cout << "Read file successfully!!" << endl;
		else 
			cout << "Read file fails!!" << endl;
		//startTime
		startwtime = MPI_Wtime();
	
		//Get bmpInfo to be sent
		height = bmpInfo.biHeight;
		width = bmpInfo.biWidth;
	}

	//Brocast height & width to other proc
	MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

	//allocate memory
	BMPData = alloc_memory(height/numprocs, width);
	RGBTRIPLE **BMPtemp = alloc_memory(height/numprocs, width);

	//create new data type
	MPI_Datatype MPI_RGB;
	MPI_Type_contiguous(3, MPI_CHAR, &MPI_RGB);
	MPI_Type_commit(&MPI_RGB);

	//global pointer for SaveData
	RGBTRIPLE *globalptr = NULL;
	if (myid == 0)
		globalptr = &(BMPSaveData[0][0]);

	//Scatter BMPData from rank 0
	int sendcount[numprocs];
	int disp[numprocs];
	for (int i = 0; i<numprocs; i++)
		sendcount[i] = height/numprocs*width;
	//sendcount[numprocs-1] = height*width - height*width*(numprocs-1)/numprocs;
	for (int i = 0; i<numprocs; i++)
		disp[i] = i*height/numprocs*width;
	MPI_Scatterv(globalptr, sendcount, disp, MPI_RGB, &(BMPtemp[0][0]), height/numprocs*width, MPI_RGB, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

	//Create edge array
	RGBTRIPLE **BMPupper = alloc_memory(1, width);
	RGBTRIPLE **BMPdown = alloc_memory(1, width);

	//Start to Smooth Data
	for(int count = 0; count < NSmooth ; count ++){

		//update the edge data
		int upper = myid > 0 ? myid-1 : numprocs-1;
		int down = myid < numprocs-1 ? myid+1 : 0;
		
		//Send upper
		if (numprocs == 1)
			BMPdown[0] = BMPtemp[0];
		else 
		{
			MPI_Send(&(BMPtemp[0][0]), width, MPI_RGB, upper, 0, MPI_COMM_WORLD);
			MPI_Recv(&(BMPdown[0][0]), width, MPI_RGB, down, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}

		//Send down
		if (numprocs == 1)
			BMPupper[0] = BMPtemp[height-1];
		else
		{
			MPI_Send(&(BMPtemp[height/numprocs-1][0]), width, MPI_RGB, down, 0, MPI_COMM_WORLD);
			MPI_Recv(&(BMPupper[0][0]), width, MPI_RGB, upper, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}

		//swap tempData and computing data
		swap(BMPtemp, BMPData);

		//Smooth
		for(int i = 0; i<height/numprocs ; i++)
			for(int j =0; j<width ; j++){
				/*********************************************************/
				/*set the position of pixel around                       */
				/*********************************************************/
				int Top = i-1;
				int Down = i+1;
				int Left = j>0 ? j-1 : width-1;
				int Right = j<width ? j+1 : 0;
				/*********************************************************/
				/*computing pixel data and rounding                      */
				/*********************************************************/

				BMPtemp[i][j].rgbBlue =  (double) (BMPData[i][j].rgbBlue+
						(Top >= 0 ? BMPData[Top][j].rgbBlue : BMPupper[0][j].rgbBlue)+
						(Down < height/numprocs ? BMPData[Down][j].rgbBlue : BMPdown[0][j].rgbBlue)+
						BMPData[i][Left].rgbBlue+BMPData[i][Right].rgbBlue)/5+0.5;
				BMPtemp[i][j].rgbGreen =  (double) (BMPData[i][j].rgbGreen+
						(Top >= 0 ? BMPData[Top][j].rgbGreen : BMPupper[0][j].rgbGreen)+
						(Down < height/numprocs ? BMPData[Down][j].rgbGreen : BMPdown[0][j].rgbGreen)+
						BMPData[i][Left].rgbGreen+BMPData[i][Right].rgbGreen)/5+0.5;
				BMPtemp[i][j].rgbRed =  (double) (BMPData[i][j].rgbRed+
						(Top >= 0 ? BMPData[Top][j].rgbRed : BMPupper[0][j].rgbRed)+
						(Down < height/numprocs ? BMPData[Down][j].rgbRed : BMPdown[0][j].rgbRed)+
						BMPData[i][Left].rgbRed+BMPData[i][Right].rgbRed)/5+0.5;
			}
	}
	
	//Gather BMPData to proc 0 BMPSaveData
	swap(BMPtemp, BMPData);
	MPI_Gatherv(&(BMPData[0][0]), height/numprocs*width, MPI_RGB, globalptr, sendcount, disp, MPI_RGB, 0, MPI_COMM_WORLD);

	//write file
	if (myid == 0)
	{
		//Get endTime and print execution time
		endwtime = MPI_Wtime();
		cout << "The execution time = "<< endwtime-startwtime <<endl ;
		
		if ( saveBMP( outfileName ) )
			cout << "Save file successfully!!" << endl;
		else
			cout << "Save file fails!!" << endl;
	}

	free(BMPData);
	free(BMPSaveData);
	MPI_Finalize();

	return 0;
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

