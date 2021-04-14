/**********
 get_aez_val.c
 
 find the aez value for a given cell index
 
 if nodata, then don't use this cell, and flag this cell
 maybe later will use the vicinity search
 
 arguments:
 int aez_array[]:		1-d array of aez values for the global grid (upper left corner origin)
 int index:				cell index for aez value retrieval
 int nrows:				number of rows in aez array
 int ncols:				number of cols in aez array
 int nodata_val:		the nodata value for the aez grid
 int *value:			address of variable to store the aez value for index

 return value:
 integer error code: OK = 0, otherwise a non-zero error code
 
 Created by Alan Di Vittorio on 4 Sep 2013
 
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

int get_aez_val(int aez_array[], int index, int nrows, int ncols, int nodata_val, int *value) {
	
	int i, j;			// indices for looping
	int temp_val;		// temporary aez value
	int count;			// radius of search ring in pixels
	int irow;			// row of cell index argument (starts at 0)
	int icol;			// column of cell index argument (starts at 0)
	double temp_dbl;	// temporary double variable
	double integer_dbl;	// used to capture the integral part of a double
	
	// these are for the currently unused search code
	int toprow, botrow, leftcol, rightcol, current_val;
	
	temp_val = aez_array[index];
	//if (temp_val == nodata_val) {
	if (0) {
		temp_dbl = index / ncols;
		modf(temp_dbl, &integer_dbl);
		irow = (int) integer_dbl;
		icol = index - irow * ncols;
		// search vicinity for nearest valid aez value
		count = 1;
		while (temp_val == nodata_val) {
			// determine rows and cols to search
			toprow = irow - count;
			if (toprow < 0) {
				toprow = 0;
			}
			botrow = irow + count;
			if (botrow >= nrows) {
				botrow = nrows - 1 ;
			}
			leftcol = icol - count;
			if (leftcol < 0) {
				leftcol = 0;
			}
			rightcol = icol + count;
			if (rightcol >= ncols) {
				rightcol = ncols - 1;
			}
			// loop over rows
			for (i = leftcol; i <= rightcol; i++) {
				if (i == toprow || i == botrow) {
					// loop over columns if at top or bottom of search ring
					for (j = icol - count; j <= icol + count; j++) {
						current_val = aez_array[i * nrows + j];
						// grab the first found value
						if (current_val != nodata_val) {
								temp_val = current_val;
								fprintf(fplog, "Found aez value for index %i at index %i\n", index, i * nrows + j);
								break; // don't need to search this row anymore
						}
					}	// end for j loop over columns
				} else {	// end if top or bottom of search ring
					current_val = aez_array[i * nrows + leftcol];
					// grab the first found value
					if (current_val != nodata_val) {
						temp_val = current_val;
						fprintf(fplog, "Found aez value for index %i at index %i\n", index, i * nrows + leftcol);
						break; // don't need to search this row anymore
					}
					current_val = aez_array[i * nrows + rightcol];
					// grab the first found value
					if (current_val != nodata_val) {
						temp_val = current_val;
						fprintf(fplog, "Found aez value for index %i at index %i\n", index, i * nrows + rightcol);
						break; // don't need to search this row anymore
					}
				}	// end else not top or bottom of search ring
				if (temp_val != nodata_val) {
					
					break;	// done searching all rows
				}
			}	// end for i loop over rows
			count++;
		}	// end while loop over search rings
	}	// end if temp_val == nodata_val
	
	if (temp_val == nodata_val) {
		missing_aez_mask[index] = 1;
	}
	
	*value = temp_val;
	
	return OK;}
