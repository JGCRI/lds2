/**********
 read_aez_new.c

 read the new aez boundaries into aez_bounds_new[NUM_CELLS]
 
 The new aezs can be arbitrarily defined
 A corresponding csv file provides integer codes and names for each AEZ

 arguments:
 args_struct in_args: the input file arguments
 rinfo_struct *raster_info: information about input raster data
 
 return value:
 integer error code: OK = 0, otherwise a non-zero error code
 
 Created by Alan Di Vittorio on 9 May 2013
 
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

int read_aez_new(args_struct in_args, rinfo_struct *raster_info) {
	
	// use this function to input data to the working grid
	
	// BIL file with one band (starts at upper left corner)
	// 4 byte signed integers
	// 5 arcmin resolution, extent = (-180,180, -90, 90), WGS84
	
	int nrows = 2160;				// num input lats
	int ncols = 4320;				// num input lons
	int ncells = nrows * ncols;		// number of input grid cells
	int nodata = -9999;             // nodata value
	int insize = 4;					// 4 byte integers for input
	double res = 5.0 / 60.0;		// resolution
	double xmin = -180.0;			// longitude min grid boundary
	double xmax = 180.0;			// longitude max grid boundary
	double ymin = -90.0;			// latitude min grid boundary
	double ymax = 90.0;				// latitude max grid boundary
	
	char fname[MAXCHAR];			// file name to open
	FILE *fpin;						// file pointer
	int num_read;					// how many values read in
	
	int err = OK;								// store error code from the write file
	char out_name[] = "aez_bounds_new.bil";		// file name for output diagnostics raster file
	
	// store file specific info
	raster_info->aez_new_nrows = nrows;
	raster_info->aez_new_ncols = ncols;
	raster_info->aez_new_ncells = ncells;
	raster_info->aez_new_nodata = nodata;
	raster_info->aez_new_insize = insize;
	raster_info->aez_new_res = res;
	raster_info->aez_new_xmin = xmin;
	raster_info->aez_new_xmax = xmax;
	raster_info->aez_new_ymin = ymin;
	raster_info->aez_new_ymax = ymax;
	
	// create file name and open it
	strcpy(fname, in_args.inpath);
	strcat(fname, in_args.aez_new_fname);
	
	if((fpin = fopen(fname, "rb")) == NULL)
	{
		fprintf(fplog,"Failed to open file %s:  read_aez_new()\n", fname);
		return ERROR_FILE;
	}
	
	// read the data
	num_read = (int) fread(aez_bounds_new, insize, ncells, fpin);
	fclose(fpin);
	if(num_read != ncells)
	{
		fprintf(fplog, "Error reading file %s: read_aez_new(); num_read=%i != overlap_cols=%i\n",
					fname, num_read, ncells);
		return ERROR_FILE;
	}
	
	if (in_args.diagnostics) {
		if ((err = write_raster_int(aez_bounds_new, ncells, out_name, in_args))) {
			fprintf(fplog, "Error writing file %s: read_aez_new()\n", out_name);
			return err;
		}
	}
	
	return OK;}
