/**********
 get_land_cells.c
 
 NOTE: the overall output land mask is determined by the intersection of hyde land, fao country, glu, and ctry87 (which determines whether an fao country is included in the output as an economic region)
 	So all the outputs are restricted to this overall mask, either spatially or thematically (the sage data are processed on the sage land base, then thematically assigned to the hyde land base)
 	The diagnostic raster outputs are based on their original land masks, and the others are based on the overall land mask:
 	The gcam region, country, ctry87, and gcam region/glu, country/glu, glu, and valid land area raster outputs represent the overall land mask, based only on valid hyde land X glu X country X ctry87 land cells
   The noland versions of these rasters include only the valid cells where there is not valid hyde land area (no valid hyde land X glu X country X ctry87)
 
 find and put the indices of land cells to process for harvested area and irrigated harvested area and water footprint
   into land_cells_sage[num_land_cells_sage] (based on sage land fraction data)
   these are used to aggregate the sage-based data from their land grid to the valid thematic boundaries
      so some of the sage pixels that are outside the valid boundaries may be included in the outputs
      but most of these pixels are along the arctic coast of eurasia
 
 find and put the indices of land cells to process for land type area (also restricted to valid output mask)
   into land_cells_hyde[num_land_cells_hyde] (based on hyde land area, which is an effecive hyde land mask)
   the hyde grid cell area matches the land area as a land mask
 
 store forest cells based on ref veg and hyde land cells, for forest land rent calculations
   these are further restricted to valid output mask during processing
 
 also store the land cells of the new aez data in land_cells_aez_new[num_land_cells_aez_new], which are used
   to create mapping files; these are also processed to match the valid output mask
 
 land cells are those that do not contain a nodata value
 
 NOTE: since harvested area can exceed physical area due to multiple cropping,
    it is dealt with separately from the land type areas based on hyde
    however, sage harvested area is normalized to hyde cropland area based on the multi-cropping ratio
 	further multi-cropping resolution happens in the gcam data processing system
    so the main land area base is from HYDE
    but calculations using the SAGE crop data will be based on the sage land cells
 
 generate land masks for diagnostics

 spatial grid initializations happen here because it is the first time that there is a loop over the working grid
 
 also initialize the area and calibration arrays to NODATA
 
 also initialize the land mask arrays to 0
 
 the num_land_cells_#### variables are initialized in init_moirai.c

 also gets the non land cells for regions, basins and countries
 
 recall that serbia and montenegro have separate raster fao code values but are processed merged
    so need to assign the proper gcam region based on the merged fao code
 
 add some diagnostics regarding the number of mismatched country and land cells
 
 area units are km^2
 
 arguments:
 args_struct in_args:	input argument structure
 rinfo_struct raster_info: information about input raster data

 return value:
	integer error code: OK = 0, otherwise a non-zero error code
 
 Created by Alan Di Vittorio on 16 May 2013
 
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
 
 Modified oct 2015 by Alan Di Vittorio
    Aligning spatial and FAO data for processing and output, using iso3 codes
    In conjunction with allowing arbitrary AEZs as input
 
 Updated jan 2016 to keep track of hyde land cells for the working grid
 
 Modified 2018 by Alan Di Vittorio
 	adding new lulc dataset
 	determining the overall valid land cells (identified with a country/region) for:
 		sage processing
 		overall output
 
 **********/

#include "moirai.h"

int get_land_cells(args_struct in_args, rinfo_struct raster_info) {
	
	// valid values in the sage land area data set determine the land cells to process for harvested area
    // correspondence with gcam regions will be determined by iso to gcam region mapping
	
	int i, j, k = 0;
	int err = OK;				// store error code from the write functions
	int fao_index;				// store the fao country index
    int scg_code = 186;         // fao code for serbia and montenegro
    int srb_code = 272;         // fao code for serbia
    int mne_code = 273;         // fao code for montenegro
    int scg_index;              // the index in the fao country info arrays of merged serbia and montenegro
    
    float temp_float;
    
    // for tracking land area
    double total_sage_land_area = 0;        // global sage land area
    double extra_sage_area = 0;             // sage land cells not covered by hyde land cells
    double new_aez_sage_area_lost = 0;      // sage cells not covered by new aez data
    double orig_aez_sage_area_lost = 0;     // sage cells not covered by orig aez data
    double potveg_sage_area_lost = 0;       // sage cells not covered by pot veg data
    double fao_sage_area_lost = 0;          // sage cells not covered by fao country data
    double fao_new_aez_sage_area_lost = 0;  // sage cells not covered by fao country data or new aez data
    double total_hyde_land_area = 0;        // global hyde land area
    double extra_hyde_area = 0;             // hyde land cells not covered by sage land cells
    double new_aez_hyde_area_lost = 0;      // hyde cells not covered by new aez data
    double orig_aez_hyde_area_lost = 0;     // hyde cells not covered by orig aez data
    double potveg_hyde_area_lost = 0;       // hyde cells not covered by pot veg data
    double fao_hyde_area_lost = 0;          // hyde cells not covered by fao country data
    double fao_new_aez_hyde_area_lost = 0;  // hyde cells not covered by fao country data or new aez data
    
	char out_name_ctry87[] = "country87_out.bil";	// output name for new country87 map
	char out_name_region[] = "region_gcam_out.bil";	// output name for new gcam region raster map
	char out_name_ctry_out[] = "country_out.bil";	// output name for new country raster map
	char out_name_ctryglu[] = "ctryglu_raster.bil"; // output name for new country/glu raster map
	char out_name_regionglu[] = "regionglu_raster.bil"; // output name for new region/glu raster map
	char out_name_glu[] = "glu_raster.bil"; // output name for new region/glu raster map
	char out_name_valid_land[] = "valid_land_area.bil"; //output name for valid land area
	
	char out_name_region_noland[] = "region_gcam_out_noland.bil";	// output name for new gcam region raster map
	char out_name_ctry_out_noland[] = "country_out_noland.bil";	// output name for new country raster map
	char out_name_ctryglu_noland[] = "ctryglu_raster_noland.bil"; // output name for new country/glu raster map
	char out_name_regionglu_noland[] = "regionglu_raster_noland.bil"; // output name for new region/glu raster map
    char out_name_glu_noland[] = "glu_raster_noland.bil"; // output name for new region/glu raster map


	float *ctryaez_raster;    // store the ctry+aez values as a raster file
	float *regionaez_raster;    // store the gcam region+aez values as a raster file
	float *region_raster;   // store the gcam region values as a raster file   
	float *country_out;    // store the output country codes as a raster file
	float *aez_out;       // store output basin codes as a raster file
    float *valid_land_area;

	float *ctryaez_raster_noland;    // store the ctry+aez values as a raster file
	float *region_raster_noland;        // store the gcam region+aez values as a raster file
	float *regionaez_raster_noland;    // store the gcam region+aez values as a raster file
	float *country_out_noland;    // store the output country codes as a raster file
	float *aez_out_noland;       // store output basin codes as a raster file

	
   
	// allocate the raster arrays
	ctryaez_raster = calloc(NUM_CELLS, sizeof(float));
	if(ctryaez_raster == NULL) {
		fprintf(fplog,"Failed to allocate memory for ctryaez_raster:  get_land_cells()\n");
		return ERROR_MEM;
	}
	regionaez_raster = calloc(NUM_CELLS, sizeof(float));
	if(regionaez_raster == NULL) {
		fprintf(fplog,"Failed to allocate memory for regionaez_raster:  get_land_cells()\n");
		return ERROR_MEM;
	}
	country_out = calloc(NUM_CELLS, sizeof(float));
	if(country_out == NULL) {
		fprintf(fplog,"Failed to allocate memory for country_out:  get_land_cells()\n");
		return ERROR_MEM;
	}

    aez_out = calloc(NUM_CELLS, sizeof(float));
	if(aez_out == NULL) {
		fprintf(fplog,"Failed to allocate memory for aez_out:  get_land_cells()\n");
		return ERROR_MEM;
	}
    
	region_raster = calloc(NUM_CELLS, sizeof(float));
	if(region_raster == NULL) {
		fprintf(fplog,"Failed to allocate memory for aez_out:  get_land_cells()\n");
		return ERROR_MEM;
	}
	
   ctryaez_raster_noland = calloc(NUM_CELLS, sizeof(float));
	if(ctryaez_raster_noland == NULL) {
		fprintf(fplog,"Failed to allocate memory for ctryaez_raster_noland:  get_land_cells()\n");
		return ERROR_MEM;
	}
	regionaez_raster_noland = calloc(NUM_CELLS, sizeof(float));
	if(regionaez_raster_noland == NULL) {
		fprintf(fplog,"Failed to allocate memory for regionaez_raster_noland:  get_land_cells()\n");
		return ERROR_MEM;
	}
	country_out_noland = calloc(NUM_CELLS, sizeof(float));
	if(country_out_noland == NULL) {
		fprintf(fplog,"Failed to allocate memory for country_out_noland:  get_land_cells()\n");
		return ERROR_MEM;
	}
	region_raster_noland = calloc(NUM_CELLS, sizeof(float));
	if(region_raster_noland == NULL) {
		fprintf(fplog,"Failed to allocate memory for region_raster_noland:  get_land_cells()\n");
		return ERROR_MEM;
	}
    
	aez_out_noland = calloc(NUM_CELLS, sizeof(float));
	if(aez_out_noland == NULL) {
		fprintf(fplog,"Failed to allocate memory for aez_out_noland:  get_land_cells()\n");
		return ERROR_MEM;
	}
    
	valid_land_area = calloc(NUM_CELLS, sizeof(float));
	if(valid_land_area == NULL) {
		fprintf(fplog,"Failed to allocate memory for valid_land_area:  get_land_cells()\n");
		return ERROR_MEM;
	}


	// loop over the all grid cells
	for (i = 0; i < NUM_CELLS; i++) {
		// initialize the land masks and country maps
		land_mask_aez_orig[i] = 0;
		land_mask_aez_new[i] = 0;
		land_mask_sage[i] = 0;
		land_mask_hyde[i] = 0;
		land_mask_fao[i] = 0;
		land_mask_potveg[i] = 0;
		land_mask_refveg[i] = 0;
		land_mask_forest[i] = 0;
        land_mask_ctryaez[i] = 0;
		country87_gtap[i] = NODATA;
        glacier_water_area_hyde[i] = NODATA;
        region_gcam[i] = NODATA;
		region_raster[i] = NODATA;
		region_raster_noland[i]= NODATA;
		ctryaez_raster[i] = NODATA;
		ctryaez_raster_noland[i] = NODATA;
		regionaez_raster[i] = NODATA;
		regionaez_raster_noland[i]= NODATA;
		country_out[i] = NODATA;
		country_out_noland[i] = NODATA;
		aez_out[i]= NODATA;
		aez_out_noland[i] = NODATA;
		
		// if valid original aez id value, then add cell index to land_mask_aez_orig
		if (aez_bounds_orig[i] != raster_info.aez_orig_nodata) {
			land_mask_aez_orig[i] = 1;
		}
		// if valid new aez id value, then add cell index to land_cells_aez_new array
		if (aez_bounds_new[i] != raster_info.aez_new_nodata) {
			land_cells_aez_new[num_land_cells_aez_new++] = i;
			land_mask_aez_new[i] = 1;
		}
		// if sage land area, then add cell index to land_cells_sage array and land_mask_sage
		if (land_area_sage[i] != raster_info.land_area_sage_nodata) {
			land_cells_sage[num_land_cells_sage++] = i;
			land_mask_sage[i] = 1;
		}
		// if hyde land area, then add cell index to land_cells_hyde array and land_mask_hyde
        // also keep track of residual water/ice area
		if (land_area_hyde[i] != raster_info.land_area_hyde_nodata) {
            temp_float = land_area_hyde[i];
            land_cells_hyde[num_land_cells_hyde++] = i;
			land_mask_hyde[i] = 1;
            if (cell_area_hyde[i] != raster_info.cell_area_hyde_nodata) {
                temp_float = cell_area_hyde[i];
                glacier_water_area_hyde[i] = cell_area_hyde[i] - land_area_hyde[i];
            }
		}
		// if fao country, then add cell index to land_mask_fao
        // and update gcam region image, only if a hyde land cell
        // valid fao/vmap0 territories with no iso3 or gcam region or gtap ctry87 will have values == NOMATCH for ctry87 and gcam regions
        // serbia and montenegro are also not assigned to a gcam region by the ctry87 file, but they need to be counted here
		//		they are, however, assigned to a region based on the iso to gcam region file
        // so leave the NOMATCH regions as the NODATA value in the gcam region image
		if ((int) country_fao[i] != raster_info.country_fao_nodata) {
			land_mask_fao[i] = 1;
		} // end if valid country fao
		// if sage pot veg, then add cell index to land_mask_potveg
		if (potveg_thematic[i] != raster_info.potveg_nodata) {
			land_mask_potveg[i] = 1;
		}
		
        // track some area differences
        if (land_mask_sage[i] == 1) {
            total_sage_land_area = total_sage_land_area + land_area_sage[i];
            if (land_mask_hyde[i] == 0) {
                extra_sage_area = extra_sage_area + land_area_sage[i];
            }
            if (land_mask_aez_new[i] == 0) {
                new_aez_sage_area_lost = new_aez_sage_area_lost + land_area_sage[i];
            }
            if (land_mask_aez_orig[i] == 0) {
                orig_aez_sage_area_lost = orig_aez_sage_area_lost + land_area_sage[i];
            }
            if (land_mask_potveg[i] == 0) {
                potveg_sage_area_lost = potveg_sage_area_lost + land_area_sage[i];
            }
            if (land_mask_fao[i] == 0) {
                fao_sage_area_lost = fao_sage_area_lost + land_area_sage[i];
            }
            // this is the actual area not used because either there is no country or no aez
            if (land_mask_fao[i] == 0 || land_mask_aez_new[i] == 0) {
                fao_new_aez_sage_area_lost = fao_new_aez_sage_area_lost + land_area_sage[i];
            }
        }
        if (land_mask_hyde[i] == 1) {
            total_hyde_land_area = total_hyde_land_area + land_area_hyde[i];
            if (land_mask_sage[i] == 0) {
                extra_hyde_area = extra_hyde_area + land_area_hyde[i];
            }
            if (land_mask_aez_new[i] == 0) {
                new_aez_hyde_area_lost = new_aez_hyde_area_lost + land_area_hyde[i];
            }
            if (land_mask_aez_orig[i] == 0) {
                orig_aez_hyde_area_lost = orig_aez_hyde_area_lost + land_area_hyde[i];
            }
            if (land_mask_potveg[i] == 0) {
                potveg_hyde_area_lost = potveg_hyde_area_lost + land_area_hyde[i];
            }
            if (land_mask_fao[i] == 0) {
                fao_hyde_area_lost = fao_hyde_area_lost + land_area_hyde[i];
            }
            // this is the actual area not used because either there is no country or no aez
            if (land_mask_fao[i] == 0 || land_mask_aez_new[i] == 0) {
                fao_new_aez_hyde_area_lost = fao_new_aez_hyde_area_lost + land_area_hyde[i];
            }
        }
        
        // the sage cell area is within 0.000229 km^2 against the available hyde cell area
        // the land areas are not directly comprable because original hyde does not include all glacier area
        //  the updated hyde land area does include much of the glacial area, but it is not perfect
        // this raster shows only where they overlap
        if (land_area_hyde[i] != raster_info.land_area_hyde_nodata && land_area_sage[i] != raster_info.land_area_sage_nodata) {
            sage_minus_hyde_land_area[i] = land_area_sage[i] - land_area_hyde[i];
        }
        
		// initialize the working area arrays
		cropland_area[i] = NODATA;
		pasture_area[i] = NODATA;
		urban_area[i] = NODATA;
		refveg_area[i] = NODATA;
		sage_minus_hyde_land_area[i] = NODATA;
		for (k = 0; k < NUM_HYDE_TYPES - NUM_HYDE_TYPES_MAIN; k++) {
			lu_detail_area[k][i] = NODATA;
		}
		
		// initialize the aez value diagnostic array
		missing_aez_mask[i] = 0;

		// get the ctry87 codes and gcam region codes to store raster maps
		// only if this is a hyde land cell, valid glu, valid country, valid ctry87
        // valid fao/vmap0 territories with no iso3 or gcam region or gtap ctry87 will have values == NOMATCH for ctry87 and gcam region
		// serbia and montenegro are also not assigned to a gcam region by the ctry87 file, but they need to be counted here
		if (land_mask_hyde[i] == 1 && land_mask_aez_new[i] == 1) {
			// fao country index
			if ((int) country_fao[i] != raster_info.country_fao_nodata) {
				fao_index = NOMATCH;
				for (j = 0; j < NUM_FAO_CTRY; j++) {
                    if (countrycodes_fao[j] == (int) country_fao[i]) {
						fao_index = j;
						break;
					}
				}	// end for j loop over fao ctry to find fao index for gtap 87 code
                if (fao_index == NOMATCH) {
                    fprintf(fplog, "Error determining fao country index for land cell: get_land_cells();country_fao %i; faoindex= %i; cellind = %i\n",country_fao[i],fao_index, i);
                    return ERROR_IND;
                }
			} else {
				// this area lost is summed in fao_hyde_area_lost above
				//fprintf(fplog, "Warning: No FAO country exists for this hyde land cell: get_land_cells(); cellind = %i\n", i);
				continue;	// no country associated with these data so don't use this cell and go to the next one
			}	// end if fao country else if gcam gis country else no country
			
            // store the ctry87 code and gcam region code and country out code in a raster
			// also store the country code, the country/glu, and the region/glu
            // leave the NOMATCH regions as the NODATA value
			// the gcam region codes have already been restricted to valid ctry87 codes, but leave the check anyway
            if (ctry2ctry87codes_gtap[fao_index] != NOMATCH) {
                country87_gtap[i] = ctry2ctry87codes_gtap[fao_index];
				if (ctry2regioncodes_gcam[fao_index] != NOMATCH) {
					region_gcam[i] = ctry2regioncodes_gcam[fao_index];
					valid_land_area[i] = land_area_hyde[i];
					region_raster[i]= (float)ctry2regioncodes_gcam[fao_index];
				}
				// fill the aez image here 
            aez_out[i] =  (float) aez_bounds_new[i];
				
				country_out[i] = (float)country_fao[i];
				// fill the ctry+aez image here
				ctryaez_raster[i] = (float) country_fao[i] * FAOCTRY2GCAMCTRYAEZID + aez_bounds_new[i];
				// fill the region+aez image here
				regionaez_raster[i] =(float) ctry2regioncodes_gcam[fao_index] * FAOCTRY2GCAMCTRYAEZID + aez_bounds_new[i];
            } else {
				// check for serbia and montenegro
				if (countrycodes_fao[fao_index] == srb_code || countrycodes_fao[fao_index] == mne_code) {
					scg_index = NOMATCH;
					for (k = 0; k < NUM_FAO_CTRY; k++) {
						if (countrycodes_fao[k] == scg_code) {
							scg_index = k;
							break;
						}
					}
					country87_gtap[i] = ctry2ctry87codes_gtap[scg_index];
					region_gcam[i] = ctry2regioncodes_gcam[scg_index];
					region_raster[i] =(float) ctry2regioncodes_gcam[scg_index];
					valid_land_area[i] = land_area_hyde[i];
					// fill the aez image here 
               aez_out[i] =  (float) aez_bounds_new[i];
					
					country_out[i] = (float)scg_code;
					// fill the ctry+aez image here
					ctryaez_raster[i] = (float)country_out[i] * FAOCTRY2GCAMCTRYAEZID + aez_bounds_new[i];
					// fill the region+aez image here
					regionaez_raster[i] = (float)region_gcam[i] * FAOCTRY2GCAMCTRYAEZID + aez_bounds_new[i];
				} // end if serbia or montenegro
			} // end else check for serbia or montenegro

		}	// end if hyde and new glu land cell (if working land cell)
        
		//New code to get no_land cells  
    	if (land_mask_hyde[i] != 1 && land_mask_aez_new[i] == 1) {
			// fao country index
			if ((int) country_fao[i] != raster_info.country_fao_nodata) {
				fao_index = NOMATCH;
				for (j = 0; j < NUM_FAO_CTRY; j++) {
                    if (countrycodes_fao[j] == (int) country_fao[i]) {
						fao_index = j;
						break;
					}
				}	// end for j loop over fao ctry to find fao index for gtap 87 code
                if (fao_index == NOMATCH) {
                    fprintf(fplog, "Error determining fao country index for land cell: get_land_cells();country_fao %i; faoindex= %i; cellind = %i\n",country_fao[i],fao_index, i);
                    return ERROR_IND;
                }
			} else {
				// this area lost is summed in fao_hyde_area_lost above
				//fprintf(fplog, "Warning: No FAO country exists for this hyde land cell: get_land_cells(); cellind = %i\n", i);
				continue;	// no country associated with these data so don't use this cell and go to the next one
			}	// end if fao country else if gcam gis country else no country
			
            // store the ctry87 code and gcam region code and country out code in a raster
			// also store the country code, the country/glu, and the region/glu
            // leave the NOMATCH regions as the NODATA value
			// the gcam region codes have already been restricted to valid ctry87 codes, but leave the check anyway
            if (ctry2ctry87codes_gtap[fao_index] != NOMATCH) {
				if (ctry2regioncodes_gcam[fao_index] != NOMATCH) {
					region_raster_noland[i] = (float)ctry2regioncodes_gcam[fao_index];
				}
				country_out_noland[i] =(float) country_fao[i];
            // fill the aez image here
            aez_out_noland[i] =  (float) aez_bounds_new[i];
				// fill the ctry+aez image here
				ctryaez_raster_noland[i] = (float) country_fao[i] * FAOCTRY2GCAMCTRYAEZID + aez_bounds_new[i];
				// fill the region+aez image here
				regionaez_raster_noland[i] = (float)ctry2regioncodes_gcam[fao_index] * FAOCTRY2GCAMCTRYAEZID + aez_bounds_new[i];
            } else {
				// check for serbia and montenegro
				if (countrycodes_fao[fao_index] == srb_code || countrycodes_fao[fao_index] == mne_code) {
					scg_index = NOMATCH;
					for (k = 0; k < NUM_FAO_CTRY; k++) {
						if (countrycodes_fao[k] == scg_code) {
							scg_index = k;
							break;
						}
					}
					region_raster_noland[i] =(float) ctry2regioncodes_gcam[scg_index];
					country_out_noland[i] = (float)scg_code;
					aez_out_noland[i]=(float)aez_bounds_new[i];
					// fill the ctry+aez image here
					ctryaez_raster_noland[i] =(float) country_out_noland[i] * FAOCTRY2GCAMCTRYAEZID + aez_bounds_new[i];
					// fill the region+aez image here
					regionaez_raster_noland[i] = (float)region_raster_noland[i] * FAOCTRY2GCAMCTRYAEZID + aez_bounds_new[i];
					//fill aex image here

				} // end if serbia or montenegro
			} // end else check for serbia or montenegro

		}	// end if hyde and new glu land cell (if working land cell)







	}	// end for i loop over all cells
	
	// write the relevant maps with the overall land mask constraints
	
    // write the new gcam region raster map
	//land
    if ((err = write_raster_float(region_raster, NUM_CELLS, out_name_region, in_args))) {
        fprintf(fplog, "Error writing file %s: get_land_cells()\n", out_name_region);
        return err;
    }
    //noland
	if ((err = write_raster_float(region_raster_noland, NUM_CELLS, out_name_region_noland, in_args))) {
        fprintf(fplog, "Error writing file %s: get_land_cells()\n", out_name_region_noland);
        return err;
    }

	// this is the map of found gtap 87 countries
	
	if ((err = write_raster_int(country87_gtap, NUM_CELLS, out_name_ctry87, in_args))) {
		fprintf(fplog, "Error writing file %s: get_land_cells()\n", out_name_ctry87);
		return err;
	}
	// this is the country map
	//land
	if ((err = write_raster_float(country_out, NUM_CELLS, out_name_ctry_out, in_args))) {
		fprintf(fplog, "Error writing file %s: get_land_cells()\n", out_name_ctry_out);
		return err;
	}
	//noland
	if ((err = write_raster_float(country_out_noland, NUM_CELLS, out_name_ctry_out_noland, in_args))) {
		fprintf(fplog, "Error writing file %s: get_land_cells()\n", out_name_ctry_out);
		return err;
	}
	
	// country+aez raster file
	//land
	if ((err = write_raster_float(ctryaez_raster, NUM_CELLS, out_name_ctryglu, in_args))) {
		fprintf(fplog, "Error writing file %s: get_land_cells()\n", out_name_ctryglu);
		return err;
	}
	//noland
	if ((err = write_raster_float(ctryaez_raster_noland, NUM_CELLS, out_name_ctryglu_noland, in_args))) {
		fprintf(fplog, "Error writing file %s: get_land_cells()\n", out_name_ctryglu);
		return err;
	}
	// region+aez raster file
	//land
	if ((err = write_raster_float(regionaez_raster, NUM_CELLS, out_name_regionglu, in_args))) {
		fprintf(fplog, "Error writing file %s: get_land_cells()\n", out_name_regionglu);
		return err;
	}
	
	//noland
	if ((err = write_raster_float(regionaez_raster_noland, NUM_CELLS, out_name_regionglu_noland, in_args))) {
		fprintf(fplog, "Error writing file %s: get_land_cells()\n", out_name_regionglu);
		return err;
	}

    // basin raster file
	//land
	if ((err = write_raster_float(aez_out, NUM_CELLS,out_name_glu, in_args))) {
		fprintf(fplog, "Error writing file %s: get_land_cells()\n", out_name_glu);
		return err;
	}
	
	//noland
	if ((err = write_raster_float(aez_out_noland, NUM_CELLS, out_name_glu_noland, in_args))) {
		fprintf(fplog, "Error writing file %s: get_land_cells()\n", out_name_glu_noland);
		return err;
	}
    
	//write out valid land area
	if ((err = write_raster_float(valid_land_area, NUM_CELLS, out_name_valid_land, in_args))) {
		fprintf(fplog, "Error writing file %s: get_land_cells()\n", out_name_valid_land);
		return err;
	}


    // write the global area tracking values to the log file
    fprintf(fplog, "\nGlobal land area tracking (km^2): get_land_cells():\n");
    fprintf(fplog, "total_sage_land_area = %f\n", total_sage_land_area);
    fprintf(fplog, "extra_sage_area = %f\n", extra_sage_area);
    fprintf(fplog, "new_aez_sage_area_lost = %f\n", new_aez_sage_area_lost);
    fprintf(fplog, "orig_aez_sage_area_lost = %f\n", orig_aez_sage_area_lost);
    fprintf(fplog, "potveg_sage_area_lost = %f\n", potveg_sage_area_lost);
    fprintf(fplog, "fao_sage_area_lost = %f\n", fao_sage_area_lost);
    fprintf(fplog, "sage area not used due to no fao country or no new aez = %f\n\n", fao_new_aez_sage_area_lost);
    fprintf(fplog, "total_hyde_land_area = %f\n", total_hyde_land_area);
    fprintf(fplog, "extra_hyde_area = %f\n", extra_hyde_area);
    fprintf(fplog, "new_aez_hyde_area_lost = %f\n", new_aez_hyde_area_lost);
    fprintf(fplog, "orig_aez_hyde_area_lost = %f\n", orig_aez_hyde_area_lost);
    fprintf(fplog, "potveg_hyde_area_lost = %f\n", potveg_hyde_area_lost);
    fprintf(fplog, "fao_hyde_area_lost = %f\n", fao_hyde_area_lost);
    fprintf(fplog, "hyde area not used due to no fao country or no new aez = %f\n\n", fao_new_aez_hyde_area_lost);
    
	if (in_args.diagnostics) {
		// aez orig land mask
		if ((err = write_raster_int(land_mask_aez_orig, NUM_CELLS, "land_mask_aez_orig.bil", in_args))) {
			fprintf(fplog, "Error writing file %s: get_land_cells()\n", "land_mask_aez_orig.bil");
			return err;
		}
		// aez new land mask
		if ((err = write_raster_int(land_mask_aez_new, NUM_CELLS, "land_mask_aez_new.bil", in_args))) {
			fprintf(fplog, "Error writing file %s: get_land_cells()\n", "land_mask_aez_new.bil");
			return err;
		}
		// sage land mask
		if ((err = write_raster_int(land_mask_sage, NUM_CELLS, "land_mask_sage.bil", in_args))) {
			fprintf(fplog, "Error writing file %s: get_land_cells()\n", "land_mask_sage.bil");
			return err;
		}
		// hyde land mask
		if ((err = write_raster_int(land_mask_hyde, NUM_CELLS, "land_mask_hyde.bil", in_args))) {
			fprintf(fplog, "Error writing file %s: get_land_cells()\n", "land_mask_hyde.bil");
			return err;
		}
		// fao land mask
		if ((err = write_raster_int(land_mask_fao, NUM_CELLS, "land_mask_fao.bil", in_args))) {
			fprintf(fplog, "Error writing file %s: get_land_cells()\n", "land_mask_fao.bil");
			return err;
		}
		// pot veg land mask
		if ((err = write_raster_int(land_mask_potveg, NUM_CELLS, "land_mask_potveg.bil", in_args))) {
			fprintf(fplog, "Error writing file %s: get_land_cells()\n", "land_mask_potveg.bil");
			return err;
		}
		// forest land mask
		if ((err = write_raster_int(land_mask_forest, NUM_CELLS, "land_mask_forest.bil", in_args))) {
			fprintf(fplog, "Error writing file %s: get_land_cells()\n", "land_mask_forest.bil");
			return err;
		}
        // hyde glacier-water area for land
        if ((err = write_raster_float(glacier_water_area_hyde, NUM_CELLS, "residual_ice_wat_area_hyde.bil", in_args))) {
            fprintf(fplog, "Error writing file %s: get_land_cells()\n", "residual_ice_wat_area_hyde.bil");
            return err;
        }
        // sage minus hyde cell area
        if ((err = write_raster_float(sage_minus_hyde_land_area, NUM_CELLS, "sage_minus_hyde_land_area.bil", in_args))) {
            fprintf(fplog, "Error writing file %s: get_land_cells()\n", "sage_minus_hyde_land_area.bil");
            return err;
        }
	}	// end if diagnostics
	
	free(ctryaez_raster);
	free(regionaez_raster);
	free(country_out);
	free(aez_out);
	free(ctryaez_raster_noland);
	free(region_raster_noland);
	free(regionaez_raster_noland);
	free(country_out_noland);
	free(aez_out_noland);
	free(region_raster);
	free(valid_land_area);
	
	return OK;}
