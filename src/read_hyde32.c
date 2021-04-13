/**********
 read_hyde32.c
 
 read one year/band of crop, total pasture, and urban data into crop_grid[NUM_CELLS], pasture_grid[NUM_CELLS], and urban_grid[NUM_CELLS]
 also read the land use detail data:
 	intensive pasture, rangeland, irr/rainfed rice/non-rice, total irrigated, total rainfed, total rice
 area is in km^2
 
 the input files are individual year arc ascii files
 47 years available: 1700 - 2000 every 10 years, 2001-2016 each year
 the input file names are determined from the hyde input type file
 the first 3 files are the total crop, total pasture, and total urban area
 the remaining 9 files are the lu detail
 
 arguments:
 args_struct in_args: the input file arguments
 rinfo_struct *raster_info: the raster info structure
 int year
 float* crop_grid:      the array to load the crop data into
 float* pasture_grid:   the array to load the pasture data into
 float* urban_grid:     the array to load the urban data into
 float** lu_detail_area:     the array to load the detailed lu data into
 
 return value:
 integer error code: OK = 0, otherwise a non-zero error code
 
 Created by Alan Di Vittorio on 16 Jan 2018
 
 Moirai Land Data System (Moirai) Copyright (c) 2019, The
 Regents of the University of California, through Lawrence Berkeley National
 Laboratory (subject to receipt of any required approvals from the U.S.
 Dept. of Energy).  All rights reserved.
 
 If you have questions about your rights to use or distribute this software,
 please contact Berkeley Lab's Intellectual Property Office at
 IPO@lbl.gov.
 
 NOTICE.  This Software was developed under funding from the U.S. Department
 of Energy and the U.S. Government consequently retains certain rights.  As
 such, the U.S. Government has been granted for itself and others acting on
 its behalf a paid-up, nonexclusive, irrevocable, worldwide license in the
 Software to reproduce, distribute copies to the public, prepare derivative
 works, and perform publicly and display publicly, and to permit other to do
 so.
 
 This file is part of Moirai.
 
 Moirai is free software: you can use it under the terms of the modified BSD-3 license (see …/moirai/license.txt)
 
 **********/

#include "moirai.h"

int read_hyde32(args_struct in_args, rinfo_struct *raster_info, int year, float* crop_grid, float* pasture_grid, float* urban_grid, float** lu_detail_area) {
	
	// use this function to input data to the working grid
	
	// historical hyde land use data
	// arc ascii files (starts at upper left corner: -180,90)
	// 4 byte float
	// 5 arcmin resolution, extent = (-180,180, -90, 90), ?WGS84?
	// input values are area in km^2
	
	// read in this info from a file
	//int nrows = 2160;				// num input lats
	//int ncols = 4320;				// num input lons
	//int ncells = nrows * ncols;		// number of input grid cells
	//int nodata = -9999;             // nodata value
	//double res = 5.0 / 60.0;		// resolution
	//double xmin = -180.0;			// longitude min grid boundary
	//double xmax = 180.0;			// longitude max grid boundary
	//double ymin = -90.0;			// latitude min grid boundary
	//double ymax = 90.0;				// latitude max grid boundary
	
	int nrows;				// num input lats
	int ncols;				// num input lons
	int ncells;				// number of input grid cells
	int nodata;             // nodata value
	double res;				// resolution
	double xmin;			// longitude min grid boundary
	double xmax;			// longitude max grid boundary
	double ymin;			// latitude min grid boundary
	double ymax;			// latitude max grid boundary
	
	int i, k;
	int sysrv;						// system return value
	
	char fname[MAXCHAR];            // file name to open
	char tmp_str[1100];          // temporary string
	FILE* fpin;
	
	char atag[] = "AD.asc";
	char lutag[] = "AD_lu.zip";
	char poptag[] = "AD_pop.zip";
	
	// get one header and set the lu info
	// the geographic parameters are the same for all the hyde files
	
	strcpy(fname, in_args.hydepath);
	strcat(fname, lutypenames_hyde[0]);
	sprintf(tmp_str, "%i%s", year, atag);
	strcat(fname, tmp_str);
	
	// if this file doesn't exist, then unzip this year's data
	if((fpin = fopen(fname, "rb")) == NULL)
	{
		// unzip the lu files
		strcpy(fname, "unzip ");
		strcat(fname, in_args.hydepath);
		sprintf(tmp_str, "%i%s -d %s", year, lutag, in_args.hydepath);
		strcat(fname, tmp_str);
		sysrv = system(fname);
		// unzip the pop files
		strcpy(fname, "unzip ");
		strcat(fname, in_args.hydepath);
		sprintf(tmp_str, "%i%s -d %s", year, poptag, in_args.hydepath);
		strcat(fname, tmp_str);
		sysrv = system(fname);
		
	} else {
		fclose(fpin);
	}
	
	// now make the unzipped file name again
	strcpy(fname, in_args.hydepath);
	strcat(fname, lutypenames_hyde[0]);
	sprintf(tmp_str, "%i%s", year, atag);
	strcat(fname, tmp_str);
	
	if((fpin = fopen(fname, "r")) == NULL)
	{
		fprintf(fplog,"Failed to open file %s:  read_hyde32()\n", fname);
		return ERROR_FILE;
	}

	// stop if header is not read properly
	//if(fscanf(fpin,"%*s%i%*[^\n]\n%*s%i%*[^\n]\n%*s%lf%*[^\n]\n%*s%lf%*[^\n]\n%*s%lf%*[^\n]\n%*s%i%*[^\n]\n",
	if(fscanf(fpin,"%*s%i%*s%i%*s%lf%*s%lf%*s%lf%*s%i%*[^\r\n]\r\n",
			  &ncols, &nrows, &xmin, &ymin, &res, &nodata) == EOF)
	{
		fprintf(stderr,"Failed to read file %s header:  read_hyde32()\n", fname);
		return ERROR_FILE;
	}
	
	ncells = nrows * ncols;
	xmax = xmin + 360;
	ymax = ymin + 180;
	
	raster_info->lu_nrows = nrows;
	raster_info->lu_ncols = ncols;
	raster_info->lu_ncells = ncells;
	raster_info->lu_nodata = nodata;
	raster_info->lu_res = res;
	raster_info->lu_xmin = xmin;
	raster_info->lu_xmax = xmax;
	raster_info->lu_ymin = ymin;
	raster_info->lu_ymax = ymax;
	
	fclose(fpin);
	
	// loop through the data files
	for (k = 0; k < NUM_HYDE_TYPES; k++) {
		
		strcpy(fname, in_args.hydepath);
		strcat(fname, lutypenames_hyde[k]);
		sprintf(tmp_str, "%i%s", year, atag);
		strcat(fname, tmp_str);
		
		
		// if this file doesn't exist, then unzip this year's data
		if((fpin = fopen(fname, "rb")) == NULL)
		{
			// unzip the lu files
			strcpy(fname, "unzip ");
			strcat(fname, in_args.hydepath);
			sprintf(tmp_str, "%i%s", year, lutag);
			strcat(fname, tmp_str);
			sysrv = system(fname);
			// unzip the pop files
			strcpy(fname, "unzip ");
			strcat(fname, in_args.hydepath);
			sprintf(tmp_str, "%i%s", year, poptag);
			strcat(fname, tmp_str);
			sysrv = system(fname);
			
		} else {
			fclose(fpin);
		}
		
		// now make the unzipped file name again
		strcpy(fname, in_args.hydepath);
		strcat(fname, lutypenames_hyde[k]);
		sprintf(tmp_str, "%i%s", year, atag);
		strcat(fname, tmp_str);
		
		if((fpin = fopen(fname, "r")) == NULL)
		{
			fprintf(fplog,"Failed to open file %s:  read_hyde32()\n", fname);
			return ERROR_FILE;
		}
		
		// skip the header
		if(fscanf(fpin,"%*[^\r\n]\r\n%*[^\r\n]\r\n%*[^\r\n]\r\n%*[^\r\n]\r\n%*[^\r\n]\r\n%*[^\r\n]\r\n") == EOF)
		{
			fprintf(stderr,"Failed to read file %s header:  read_hyde32()\n", fname);
			return ERROR_FILE;
		}
		
		// if crop, pasture, or urban totals, put into explicit arrays
		// otherwise put into lu_detail_area
		
		// loop over all values in file
		for(i = 0; i < ncells; i++)
		{
			if (k == 0) {
				// read single value
				if(fscanf(fpin, "%f", &urban_grid[i]) == EOF)
				{
					fprintf(stderr,"Failed to read data value %i, file %s:  read_hyde32()\n", i, fname);
					return ERROR_FILE;
				}
			} else if (k == 1) {
				// read single value
				if(fscanf(fpin, "%f", &crop_grid[i]) == EOF)
				{
					fprintf(stderr,"Failed to read data value %i, file %s:  read_hyde32()\n", i, fname);
					return ERROR_FILE;
				}
			} else if (k == 2) {
				// read single value
				if(fscanf(fpin, "%f", &pasture_grid[i]) == EOF)
				{
					fprintf(stderr,"Failed to read data value %i, file %s:  read_hyde32()\n", i, fname);
					return ERROR_FILE;
				}
			} else {
				// read single value
				if(fscanf(fpin, "%f", &lu_detail_area[k - NUM_HYDE_TYPES_MAIN][i]) == EOF)
				{
					fprintf(stderr,"Failed to read data value %i, file %s:  read_hyde32()\n", i, fname);
					return ERROR_FILE;
				}
			}
		} // end i loop over ncells
		
		fclose(fpin);
	} // end k loop over hyde files
	
	return OK;}
