#include <mpi.h>
#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "bmp.h"

using namespace std;

//¿¿¿¿¿¿¿¿¿
#define NSmooth 1000

/*********************************************************/
/*¿¿¿¿¿                                             */
/*  bmpHeader    ¿BMP¿¿¿¿                          */
/*  bmpInfo      ¿BMP¿¿¿¿                          */
/*  **BMPSaveData¿¿¿¿¿¿¿¿¿¿¿¿               */
/*  **BMPData    ¿¿¿¿¿¿¿¿¿¿¿¿¿¿           */
/*********************************************************/
BMPHEADER bmpHeader;                        
BMPINFO bmpInfo;
RGBTRIPLE **BMPSaveData = NULL;                                               
RGBTRIPLE **BMPData = NULL;                                                   

/*********************************************************/
/*¿¿¿¿                                             */
/*  readBMP    ¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿BMPSavaData*/
/*  saveBMP    ¿¿¿¿¿¿¿¿¿¿¿¿BMPSaveData¿¿  */
/*  swap       ¿¿¿¿¿¿¿                           */
/*  **alloc_memory¿¿¿¿¿¿¿Y*X¿¿                 */
/*********************************************************/
int readBMP( char *fileName);        //read file
int saveBMP( char *fileName);        //save file
void swap(RGBTRIPLE *a, RGBTRIPLE *b);
RGBTRIPLE **alloc_memory( int Y, int X );        //allocate memory
MPI_Datatype MPI_RGB;

int main(int argc,char *argv[])
{
	/*********************************************************/
	/*¿¿¿¿¿                                             */
	/*  *infileName  ¿¿¿¿¿                             */
	/*  *outfileName ¿¿¿¿¿                             */
	/*  startwtime   ¿¿¿¿¿¿¿                         */
	/*  endwtime     ¿¿¿¿¿¿¿                         */
	/*********************************************************/
	char *infileName = "input.bmp";
	char *outfileName = "output2.bmp";
	double startwtime = 0.0, endwtime=0;
	int numprocs, myid;
	int myheight, err;

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);
	
	//cut array
	if (myid == 0)
	{
		startwtime = MPI_Wtime();

		//readfile
		if ( readBMP( infileName) )
			cout << "Read file successfully!!" << endl;
		else 
			cout << "Read file fails!!" << endl;
		MPI_Type_contiguous(bmpInfo.biWidth, MPI_CHAR, &MPI_RGB);
		MPI_Type_commit(&MPI_RGB);

		BMPData = alloc_memory( bmpInfo.biHeight/numprocs, bmpInfo.biWidth);
		MPI_Scatter(BMPSaveData, bmpInfo.biHeight/numprocs, MPI_RGB, BMPData, bmpInfo.biHeight/numprocs, MPI_RGB, 0, MPI_COMM_WORLD);
	} else {
		BMPData = alloc_memory( bmpInfo.biHeight/numprocs, bmpInfo.biWidth);
		MPI_Scatter(BMPSaveData, bmpInfo.biHeight/numprocs, MPI_RGB, BMPData, bmpInfo.biHeight/numprocs, MPI_RGB, 0, MPI_COMM_WORLD);
	}

	//¿¿¿¿¿¿¿¿¿
	for(int count = 0; count < NSmooth ; count ++){
		//update the edge data
		//get upedge
		//get down edge

		//Smooth
		for(int i = 0; i<bmpInfo.biHeight ; i++)
			for(int j =0; j<bmpInfo.biWidth ; j++){
				/*********************************************************/
				/*¿¿¿¿¿¿¿¿¿¿¿                                 */
				/*********************************************************/
				int Top = i>0 ? i-1 : bmpInfo.biHeight-1;
				int Down = i<bmpInfo.biHeight-1 ? i+1 : 0;
				int Left = j>0 ? j-1 : bmpInfo.biWidth-1;
				int Right = j<bmpInfo.biWidth-1 ? j+1 : 0;

				/*********************************************************/
				/*¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿¿                       */
				/*********************************************************/
				BMPSaveData[i][j].rgbBlue =  (double) (BMPData[i][j].rgbBlue+BMPData[Top][j].rgbBlue+BMPData[Down][j].rgbBlue+BMPData[i][Left].rgbBlue+BMPData[i][Right].rgbBlue)/5+0.5;
				BMPSaveData[i][j].rgbGreen =  (double) (BMPData[i][j].rgbGreen+BMPData[Top][j].rgbGreen+BMPData[Down][j].rgbGreen+BMPData[i][Left].rgbGreen+BMPData[i][Right].rgbGreen)/5+0.5;
				BMPSaveData[i][j].rgbRed =  (double) (BMPData[i][j].rgbRed+BMPData[Top][j].rgbRed+BMPData[Down][j].rgbRed+BMPData[i][Left].rgbRed+BMPData[i][Right].rgbRed)/5+0.5;
			}
	}

	//¿¿¿¿
	if ( saveBMP( outfileName ) )
		cout << "Save file successfully!!" << endl;
	else
		cout << "Save file fails!!" << endl;

	//¿¿¿¿¿¿¿¿¿¿¿¿¿¿
	endwtime = MPI_Wtime();
	cout << "The execution time = "<< endwtime-startwtime <<endl ;

	free(BMPData);
	free(BMPSaveData);
	MPI_Finalize();

	return 0;
}

/*********************************************************/
/* ¿¿¿¿                                              */
/*********************************************************/
int readBMP(char *fileName)
{
	//¿¿¿¿¿¿¿¿	
	ifstream bmpFile( fileName, ios::in | ios::binary );

	//¿¿¿¿¿¿
	if ( !bmpFile ){
		cout << "It can't open file!!" << endl;
		return 0;
	}

	//¿¿BMP¿¿¿¿¿¿¿
	bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );

	//¿¿¿¿¿BMP¿¿
	if( bmpHeader.bfType != 0x4d42 ){
		cout << "This file is not .BMP!!" << endl ;
		return 0;
	}

	//¿¿BMP¿¿¿
	bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );

	//¿¿¿¿¿¿¿¿¿24 bits
	if ( bmpInfo.biBitCount != 24 ){
		cout << "The file is not 24 bits!!" << endl;
		return 0;
	}

	//¿¿¿¿¿¿¿¿4¿¿¿
	while( bmpInfo.biWidth % 4 != 0 )
		bmpInfo.biWidth++;

	//¿¿¿¿¿¿¿
	BMPSaveData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);

	//¿¿¿¿¿¿
	//for(int i = 0; i < bmpInfo.biHeight; i++)
	//	bmpFile.read( (char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE));
	bmpFile.read( (char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight);

	//¿¿¿¿
	bmpFile.close();

	return 1;

}
/*********************************************************/
/* ¿¿¿¿                                              */
/*********************************************************/
int saveBMP( char *fileName)
{
	//¿¿¿¿¿BMP¿¿
	if( bmpHeader.bfType != 0x4d42 ){
		cout << "This file is not .BMP!!" << endl ;
		return 0;
	}

	//¿¿¿¿¿¿¿¿
	ofstream newFile( fileName,  ios:: out | ios::binary );

	//¿¿¿¿¿¿
	if ( !newFile ){
		cout << "The File can't create!!" << endl;
		return 0;
	}

	//¿¿BMP¿¿¿¿¿¿¿
	newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );

	//¿¿BMP¿¿¿
	newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );

	//¿¿¿¿¿¿
	//for( int i = 0; i < bmpInfo.biHeight; i++ )
	//        newFile.write( ( char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE) );
	newFile.write( ( char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight );

	//¿¿¿¿
	newFile.close();

	return 1;

}


/*********************************************************/
/* ¿¿¿¿¿¿¿¿¿Y*X¿¿¿                           */
/*********************************************************/
RGBTRIPLE **alloc_memory(int Y, int X )
{        
	//
	RGBTRIPLE **temp = new RGBTRIPLE *[ Y ];
	RGBTRIPLE *temp2 = new RGBTRIPLE [ Y * X ];
	memset( temp, 0, sizeof( RGBTRIPLE ) * Y);
	memset( temp2, 0, sizeof( RGBTRIPLE ) * Y * X );

	//¹ï¨C­Ó«ü¼Ð°}¦C¸Ìªº«ü¼Ð«Å§i¤@­Óªø«×¬°Xªº°}¦C 
	for( int i = 0; i < Y; i++){
		temp[ i ] = &temp2[i*X];
	}

return temp;

}
/*********************************************************/
/* ¥æ´«¤G­Ó«ü¼Ð                                          */
/*********************************************************/
void swap(RGBTRIPLE *a, RGBTRIPLE *b)
{
	RGBTRIPLE *temp;
	temp = a;
	a = b;
	b = temp;
}

