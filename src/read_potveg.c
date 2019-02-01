/**********
 read_potveg.c
 
 read the potveg thematic data into potveg[NUM_CELLS]
 
 arguments:
 args_struct in_args: the input file arguments
 rinfo_struct *raster_info: information about input raster data

 return value:
 integer error code: OK = 0, otherwise a non-zero error code
 
 Created by Alan Di Vittorio on 15 May 2013
 Copyright 2018 Alan Di Vittorio, Lawrence Berkeley National Laboratory, All rights reserved
 
 This file is part of Moirai.
 
 Moirai is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. Moirai is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with Moirai (/moirai/docs/COPYING.txt). If not, see <https://www.gnu.org/licenses/>.
 
 **********/

#include "moirai.h"

int read_potveg(args_struct in_args, rinfo_struct *raster_info) {
	
	// use this function to input data to the working grid
	
	// SAGE 2000 potveg data
	// converted the ascii grid to a binary bil file
	// 5 arcmin resolution, extent = (-180,180, -90, 90), ?WGS84?
	// read in integer values
	// input units are classes 1 - 15
	// working units are classes 1 - 15
	
	int nrows = 2160;				// num input lats
	int ncols = 4320;				// num input lons
	int ncells = nrows * ncols;		// number of input grid cells
    int insize = 4;                 // 4 byte integers
	int nodata = -9999;				// nodata value
	double res = 5.0 / 60.0;		// resolution
	double xmin = -180.0;			// longitude min grid boundary
	double xmax = 180.0;			// longitude max grid boundary
	double ymin = -90.0;			// latitude min grid boundary
	double ymax = 90.0;				// latitude max grid boundary
	
	char fname[MAXCHAR];			// file name to open
	FILE *fpin;						// file pointer
	int num_read;					// how many values read in
	
	int err = OK;									// store error code from the write file
	char out_name[] = "potveg_thematic.bil";		// diagnositic output raster file name
	
	// store file specific info
	raster_info->potveg_nrows = nrows;
	raster_info->potveg_ncols = ncols;
	raster_info->potveg_ncells = ncells;
	raster_info->potveg_nodata = nodata;
	raster_info->potveg_res = res;
	raster_info->potveg_xmin = xmin;
	raster_info->potveg_xmax = xmax;
	raster_info->potveg_ymin = ymin;
	raster_info->potveg_ymax = ymax;
	
	// create file name and open it
	strcpy(fname, in_args.inpath);
	strcat(fname, in_args.potveg_fname);
	
    if((fpin = fopen(fname, "rb")) == NULL)
    {
        fprintf(fplog,"Failed to open file %s:  get_cell_area()\n", fname);
        return ERROR_FILE;
    }
    
    // read the data
    num_read = fread(potveg_thematic, insize, ncells, fpin);
    fclose(fpin);
    if(num_read != ncells)
    {
        fprintf(fplog, "Error reading file %s: get_cell_area(); num_read=%i != ncells=%i\n",
                fname, num_read, ncells);
        return ERROR_FILE;
    }
    fclose(fpin);

	if (in_args.diagnostics) {
		if ((err = write_raster_int(potveg_thematic, ncells, out_name, in_args))) {
			fprintf(fplog, "Error writing file %s: read_potveg()\n", out_name);
			return err;
		}
	}
	
	return OK;
}