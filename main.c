#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hdf.h>
#include <mfhdf.h>
#include <hdf5.h>

/* #define DEBUG */


int main(int ac, char **av)
{

  char input_file_path[256], output_file_path[256];
  char hdf4sds_name[256], hdf4sds_attr_name[256];
  char hdf4sds_str_attr_buff[256];
  
  
  int hdf4sd_id, hdf4access_mode, hdf4status, hdf4n_datasets, hdf4n_file_attrs,
    hdf4sds_id, hdf4sds_index, hdf4sds_rank, hdf4sds_data_type,
    hdf4sds_n_attrs, hdf4sds_attr_data_type, hdf4sds_attr_n_values,
    hdf4sds_num_elements;

  int hdf4sds_dim_sizes[8], hdf4sds_read_start[8], hdf4sds_read_stride[8];

  float *hdf4sds_flt_buff;

  hid_t hdf5file_id, hdf5dataset_id, hdf5space_id, hdf5attr_id,
    hdf5attr_space_id, hdf5str_type;
  herr_t hdf5status;
  hsize_t hdf5dataset_dims[8], hdf5attr_size;


  /* Get input file name from the command line, and create output file name */

  if (ac < 2) {
    fprintf(stderr, "Usage is: hdf4tohdf5 inputfile outputfile\n");
    fprintf(stderr, "If no output file is given, inputfile.h5 will be produced.\n");
    return 1;
  }
  strcpy(input_file_path, av[1]);

  if (ac > 2) {
    strcpy(output_file_path, av[2]);
  } else {
    strcpy(output_file_path, input_file_path);
    strcat(output_file_path, ".h5");
  }

  /* Open hdf4 file */

  hdf4access_mode = DFACC_READ;
  hdf4sd_id = SDstart(input_file_path, hdf4access_mode);

  hdf4status = SDfileinfo(hdf4sd_id, &hdf4n_datasets, &hdf4n_file_attrs);
  if (hdf4status == FAIL) {
    fprintf(stderr, "Reading hdf4 file failed.  Returning.\n");
    return 1;
  }

  
#ifdef DEBUG
  printf("Id: %i, Datasets: %i, File_attrs: %i\n", hdf4sd_id, hdf4n_datasets,
	 hdf4n_file_attrs);
#endif



  /* Open hdf5 file */
 
  hdf5file_id = H5Fcreate(output_file_path, H5F_ACC_TRUNC, H5P_DEFAULT, 
			  H5P_DEFAULT);
  
  if (hdf5file_id < 0) {
    fprintf(stderr, "Failed to open HDF5 file %s for writing.\n",
	    output_file_path);
    return 1;
  }


  /* Loop through datasets */

  for(hdf4sds_index = 0; hdf4sds_index < hdf4n_datasets; hdf4sds_index++) {
    hdf4sds_id = SDselect(hdf4sd_id, hdf4sds_index);
    hdf4status = SDgetinfo(hdf4sds_id, hdf4sds_name, &hdf4sds_rank,
			 hdf4sds_dim_sizes, &hdf4sds_data_type,
			 &hdf4sds_n_attrs);
#ifdef DEBUG    
    printf("\nsds_id: %i, sds_name: %s, sds_rank: %i, sds_data_type: %i,\nn_attrs_: %i, dim_sizes: %i %i %i\n", hdf4sds_id, hdf4sds_name, hdf4sds_rank, hdf4sds_data_type, hdf4sds_n_attrs, hdf4sds_dim_sizes[0], hdf4sds_dim_sizes[1], hdf4sds_dim_sizes[2]);
#endif

    /* Get data from SDS into buffer */
    hdf4sds_num_elements = hdf4sds_dim_sizes[0];
    hdf4sds_read_start[0] = 0;
    hdf4sds_read_stride[0] = 1;
    if (hdf4sds_rank > 1){
      for(int i=1; i < hdf4sds_rank; i++) {
	hdf4sds_num_elements = hdf4sds_num_elements*hdf4sds_dim_sizes[i];
	hdf4sds_read_start[i] = 0;
	hdf4sds_read_stride[i] = 1;
      }
    }
#ifdef DEBUG
    printf("Total number of elements for this sds: %i\n",
	   hdf4sds_num_elements);      
#endif    

    /* Allocate the buffer to read in the data */
    hdf4sds_flt_buff = malloc((sizeof (float))*hdf4sds_num_elements);

    hdf4status = SDreaddata(hdf4sds_id, hdf4sds_read_start,
			    hdf4sds_read_stride, hdf4sds_dim_sizes,
			    (VOIDP)hdf4sds_flt_buff);
    

    /* Have all of the data from the hdf4 file,
       now need to load it into hdf5 */

    for (int i=0; i < hdf4sds_rank; i++) {
          hdf5dataset_dims[i] = (hsize_t)hdf4sds_dim_sizes[i];
    }

    hdf5space_id = H5Screate_simple(hdf4sds_rank, hdf5dataset_dims,
				    hdf5dataset_dims);
    if (hdf4sds_data_type == 5) { //Floats
      hdf5dataset_id = H5Dcreate(hdf5file_id, hdf4sds_name, H5T_IEEE_F32LE,
				 hdf5space_id, H5P_DEFAULT);
      
      hdf5status = H5Dwrite(hdf5dataset_id, H5T_IEEE_F32LE, H5S_ALL, H5S_ALL, 
			    H5P_DEFAULT, hdf4sds_flt_buff);

      /* We've finished reading in the data. Free the hdf4 data buffer. */
      free(hdf4sds_flt_buff);

      /*Assign the HDF5 attributes */
      
      for(int i=0; i < hdf4sds_n_attrs; i++) {
	hdf4status = SDattrinfo(hdf4sds_id, i, hdf4sds_attr_name,
			      &hdf4sds_attr_data_type, &hdf4sds_attr_n_values);
#ifdef DEBUG
	printf("Attribute %i: Name = %s, Data type = %i, N_values = %i\n", 
	       i, hdf4sds_attr_name, hdf4sds_attr_data_type,
	       hdf4sds_attr_n_values);
#endif	
	if(hdf4sds_attr_data_type == 4) { /* Read the string attributes */
	  hdf4status = SDreadattr(hdf4sds_id, i, (VOIDP)hdf4sds_str_attr_buff);
	  hdf4sds_str_attr_buff[hdf4sds_attr_n_values]='\0'; 
#ifdef DEBUG
	  printf("Value: %s\n", hdf4sds_str_attr_buff);
#endif
	  hdf5attr_size = (hsize_t)hdf4sds_attr_n_values;
	  
	  hdf5attr_space_id = H5Screate(H5S_SCALAR);
	  hdf5str_type = H5Tcopy(H5T_C_S1);
	  hdf5status = H5Tset_size(hdf5str_type,
				   (size_t)hdf4sds_attr_n_values);
	  hdf5attr_id = H5Acreate(hdf5dataset_id, hdf4sds_attr_name,
				  hdf5str_type, hdf5attr_space_id,
				  H5P_DEFAULT);
	  hdf5status =  H5Awrite(hdf5attr_id, hdf5str_type,
				 hdf4sds_str_attr_buff);
	  hdf5status = H5Aclose(hdf5attr_id);
	}
      }
      hdf5status = H5Dclose(hdf5dataset_id);        
    }
    hdf5status = H5Sclose(hdf5space_id);				
    hdf4status = SDendaccess(hdf4sds_id);
  }



  /* Close access to hdf4 file */

  hdf4status = SDend(hdf4sd_id);



  /* Close access to HDF5 file */
  
  hdf5status = H5Fclose(hdf5file_id);

  return 0;
}
