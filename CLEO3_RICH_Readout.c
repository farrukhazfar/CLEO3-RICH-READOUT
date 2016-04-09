/**********************************************/
/*                                            */
/*    Unpacking Routines for RICH DAQ data    */
/*    Created by      Farrukh Azfar           */
/*    and             Alexander Efimov        */
/*                                            */
/**********************************************/

#include<stdio.h>
#include<stdlib.h>

#define OSF1           0
#define DEBUG          0
#define MX_BOARDS     12
#define MX_ADC        15
#define MX_CHANNELS  128

int record_length;
int run_number;
int date_start_run;
int date_end_run;
int date_event;
int time_start_run;
int time_end_run;
int time_event;
int version_number;
int number_events;
int number_boards;
int board_id;
int board_constant;
int board_length;
int record_type;
FILE *fd;

/************************************************************************
 *                                                                      *
 *                   Swap bytes in LONG INT word (*4)                   *
 *                                                                      *
 ************************************************************************/

void swap_bytes_l4 (char *word)
{
  char w[3];

  w[0] = word[0];
  w[1] = word[1];
  w[2] = word[2];
  w[3] = word[3];
  word[0] = w[3];
  word[1] = w[2];
  word[2] = w[1];
  word[3] = w[0];
}

/************************************************************************
 *                                                                      *
 *                   Swap bytes in SHORT INT word (*2)                  *
 *                                                                      *
 ************************************************************************/

void swap_bytes_l2 (char *word)
{
  char w[1];

  w[0] = word[0];
  w[1] = word[1];
  word[0] = w[1];
  word[1] = w[0];
}

/************************************************************************
 *                                                                      *
 *                       Read Constants Record                          *
 *                                                                      *
 ************************************************************************/

void read_constant_record ()
{
  int boards;
  FILE *const_file;
  char rich[3];

  const_file = fopen ("constants.dat", "wb");
  fread ((void*)&record_length, 4, 1, fd);
  fread ((void*)&run_number, 4, 1, fd);
  fread ((void*)&date_start_run, 4, 1, fd);
  fread ((void*)&time_start_run, 4, 1, fd);
  fread ((void*)&version_number, 4, 1, fd);
  fread ((void*)&number_boards, 4, 1, fd);
  if (OSF1) {
    swap_bytes_l4 ((void*)&record_length);
    swap_bytes_l4 ((void*)&run_number);
    swap_bytes_l4 ((void*)&date_start_run);
    swap_bytes_l4 ((void*)&time_start_run);
    swap_bytes_l4 ((void*)&version_number);
    swap_bytes_l4 ((void*)&number_boards);
  }
  fprintf (const_file, "%x", record_type);
  fprintf (const_file, "%x", record_length);
  fprintf (const_file, "%x", run_number);
  fprintf (const_file, "%x", date_start_run);
  fprintf (const_file, "%x", time_start_run);
  fprintf (const_file, "%x", version_number);
  fprintf (const_file, "%x", number_boards);
  if (DEBUG) {
    printf (" ... Record length    : %d \n", record_length);
    printf (" ... Run number       : %d \n", run_number);
    printf (" ... Date RUN         : %d \n", date_start_run);
    printf (" ... Time RUN         : %d \n", time_start_run);
    printf (" ... Version          : %d \n", version_number);
    printf (" ... Number of boards : %d \n", number_boards);
  }
  boards = number_boards;
  while (boards > 0) {
    boards--;
    fread ((void*)&board_id, 4, 1, fd);
    fread ((void*)&board_length, 4, 1, fd);
    if (OSF1) {
      swap_bytes_l4 ((void*)&board_id);
      swap_bytes_l4 ((void*)&board_length);
    }
    fprintf (const_file,"%x", board_id);
    fprintf (const_file, "%x", board_length);
    while (board_length > 0) {
      fread ((void*)&board_constant, 4, 1, fd);
      if (OSF1) {
	swap_bytes_l4 ((void*)&board_constant);
      }
      fprintf (const_file, "%x", board_constant);
      board_length -= 4;
    }
  }  
  fread ((void*)rich, 4, 1, fd);
  fprintf (const_file,"%x", rich);
  close (const_file);
}

/************************************************************************
 *                                                                      *
 *                          Begin RUN Record                            *
 *                                                                      *
 ************************************************************************/

void read_begin_run_record ()
{
  int board;
  char rich[4];

  fread ((void*)&record_length, 4, 1, fd);
  fread ((void*)&run_number, 4, 1, fd);
  fread ((void*)&date_start_run, 4, 1, fd);
  fread ((void*)&time_start_run, 4, 1, fd);
  fread ((void*)&version_number, 4, 1, fd);
  fread ((void*)&number_boards, 4, 1, fd);
  if (OSF1) {
    swap_bytes_l4 ((void*)&record_length);
    swap_bytes_l4 ((void*)&run_number);
    swap_bytes_l4 ((void*)&date_start_run);
    swap_bytes_l4 ((void*)&time_start_run);
    swap_bytes_l4 ((void*)&version_number);
    swap_bytes_l4 ((void*)&number_boards);
  }
  if (DEBUG) {
    printf (" ... Record length    : %d \n", record_length);
    printf (" ... Run number       : %d \n", run_number);
    printf (" ... Date RUN         : %d \n", date_start_run);
    printf (" ... Time RUN         : %d \n", time_start_run);
    printf (" ... Version          : %d \n", version_number);
    printf (" ... Number of boards : %d \n", number_boards);
  }
  board = number_boards;
  while (board-- >0) {
    fread ((void*)&board_id, 4, 1, fd);
    if (OSF1) {
      swap_bytes_l4 ((void*)&board_id);
    }
    if (DEBUG) {
      printf ("Board id : %d \n",board_id);
    }
  }
  fread ((void*)rich, 4, 1, fd);
  rich[4] = '\0';
  if (DEBUG) {
    printf ("End of record: %s \n", rich);
  }
}

/************************************************************************
 *                                                                      *
 *                           Event Record                               *
 *                                                                      *
 ************************************************************************/

int read_event_record (int* adc_counts, int* tdc, int* event_num, 
                       int* recl_adc, int* recl_tdc, int* date, int* time)
{
  int board_location, number_words, event_number;
  int adc_word, camac_length, datum, number_boards_left, recl;
  short tdc_word;
  int board, adc, channel, pulse;
  char rich[4];
  long int i, j;
    
  fread ((void*)&record_length, 4, 1, fd);
  fread ((void*)&run_number, 4, 1, fd);
  fread ((void*)&date_event, 4, 1, fd);
  *date=date_event;
  fread ((void*)&time_event,4,1,fd);
  *time=time_event;
  fread ((void*)&version_number, 4, 1, fd);
  fread ((void*)&event_number, 4, 1, fd);
  *event_num=event_number;
  if (OSF1) {
    swap_bytes_l4 ((void*)&record_length);
    recl =record_length;
    swap_bytes_l4 ((void*)&run_number);
    swap_bytes_l4 ((void*)&date_event);
    *date=date_event;
    swap_bytes_l4 ((void*)&time_event);
    *time=time_event;
    swap_bytes_l4 ((void*)&version_number);
    swap_bytes_l4 ((void*)&event_number);
    *event_num=event_number;
  }
  if (DEBUG) {
    printf (" ... Record length    : %d \n", record_length);
    printf (" ... Run number       : %d \n", run_number);
    printf (" ... Date event       : %d \n", date_event);
    printf (" ... Time event       : %d \n", time_event);
    printf (" ... Version          : %d \n", version_number);
    printf (" ... Number of boards : %d \n", number_boards);
    printf (" ... Event Number     : %d \n", *event_num);
  }

  recl=0;
  number_boards_left=number_boards;

  while (number_boards_left > 0) {
    fread ((void*)&board_location, 4, 1, fd);
    fread ((void*)&number_words, 4, 1, fd);
    recl+=8;
    if (OSF1) {
      swap_bytes_l4 ((void*)&board_location);
      swap_bytes_l4 ((void*)&number_words);
    }
    datum = 4 * (number_words & 0x00000fff);
    if (DEBUG) {
      printf ("Location of board: %d \n",
             (board_location >> 27) & 0x1f);
      printf ("Length of data in bytes: %d \n", datum);
    }

    number_boards_left--;
    while (datum > 0) {
      fread ((void*)&adc_word, 4, 1, fd);
      if (OSF1) {
	swap_bytes_l4 ((void*)&adc_word);
      }
      board   = (adc_word >> 27) & 0x1f;
      board  -= 1;
      adc     = (adc_word >> 23) & 0x0f;
      channel = (adc_word >> 16) & 0x7f;
      pulse   = (adc_word & 0x0000ffff);
      if (channel < 0 || channel > MX_CHANNELS-1) {
	printf (" Wrong channel number: %d \n", channel);
	return 1;
      }
      if (adc < 0 || adc > MX_ADC-1) {
	printf (" Wrong ADC number: %d \n", adc);
	return 1;
      }
      if (board < 0 || board > MX_BOARDS-1) {
	printf (" Wrong board number: %d \n", board);
	return 1;
      }
      i = (board * MX_ADC + adc) * MX_CHANNELS + channel;
      adc_counts[i] = pulse;
/*
      if (DEBUG) {
	printf (" >>> Board number   : %d \n", board);
	printf (" >>> ADC number     : %d \n", adc);
	printf (" >>> Channel number : %d \n", channel);
	printf (" >>> ADC counts     : %d \n", pulse);
      }
*/

      datum -= 4;
      recl+=4;
    }
    fread ((void*)&board_location, 4, 1, fd);
    fread ((void*)&number_words, 4, 1, fd);
    if (OSF1) {
      swap_bytes_l4 ((void*)&board_location);
      swap_bytes_l4 ((void*)&number_words);
    }
    datum = 4 * (number_words & 0x00000fff);
    if (DEBUG) {
      printf ("Board location: %d \n",
             (board_location >> 27) & 0x1f);
      printf ("Data length: %d \n", datum);
    }
          recl+=8;
  }	
	
  fread ((void*)&camac_length, 4, 1, fd);		
  *recl_tdc=camac_length;
  *recl_adc=recl;

  if (OSF1) {
    swap_bytes_l4 ((void*)&camac_length);
    *recl_tdc=camac_length;
    *recl_adc=record_length-camac_length;
  }

  j=0;
  while (camac_length > 0) {    
    fread ((void*)&tdc_word, 2, 1, fd);
    if (OSF1) {
      swap_bytes_l2 ((void*)&tdc_word);
    }
    tdc[j] = tdc_word;
    j++;
    camac_length -= 2;
  }
  
  fread ((void*)rich, 4, 1, fd);
  rich[4] = '\0';
  if (DEBUG) {
    printf ("Length of Camac data: %d \n", camac_length);
    printf ("Footer in ASCII : %s \n", rich);
  }
  return 0;
}

/************************************************************************
 *                                                                      *
 *                         End RUN Record                               *
 *                                                                      *
 ************************************************************************/

void read_endrun_record ()
{
  char rich[3];

  fread ((void*)&record_length, 4, 1, fd);
  fread ((void*)&run_number, 4, 1, fd);
  fread ((void*)&date_end_run, 4, 1, fd);
  fread ((void*)&time_end_run, 4, 1, fd);
  fread ((void*)&version_number, 4, 1, fd);
  fread ((void*)&number_events, 4, 1, fd);
  fread ((void*)rich, 4, 1, fd);
  if (OSF1) {
    swap_bytes_l4 ((void*)&record_length);
    swap_bytes_l4 ((void*)&run_number);
    swap_bytes_l4 ((void*)&date_end_run);
    swap_bytes_l4 ((void*)&time_end_run);
    swap_bytes_l4 ((void*)&version_number);
    swap_bytes_l4 ((void*)&number_events);
  }
  if (DEBUG) {
    printf ("Length of record %x \n", record_length);
    printf ("Run # %x \n", run_number);
    printf ("Date when run ended %x \n", date_end_run);
    printf ("Time when run ended %x \n", time_end_run);
    printf ("Version # %x \n",version_number);
    printf ("number of events in run %x \n", number_events);
    printf ("Footer in ASCII %s \n",rich);
  }
}

/************************************************************************
 *                                                                      *
 *                    RICH_OPEN_RUN procedure                           *
 *                                                                      *
 ************************************************************************/

int rich_open_run_ (char *filename, int* run_num)
{
  int record_type;
  if (DEBUG) {
    printf ("Input file: %s \n", filename);
  }
  fd = fopen (filename, "rb");    /* open file */
  if (fd==NULL) {
    printf ("Error: opening file %s \n", filename);
    exit(1);
  }
  fread ((void*)&record_type, 4, 1, fd);
  if (OSF1) {
    swap_bytes_l4 ((void*)&record_type);
  }
  if (DEBUG) {
    printf ("Record type = %x \n", record_type);
    printf (" $$$ Begin RUN record $$$ \n");
  }
  if (record_type =! 0xffff0001)  {
    printf ("Error: no begin RUN record \n");
    return 1;
  }
  read_begin_run_record ();
  *run_num=run_number;
  return 0;
}

/************************************************************************
 *                                                                      *
 *                  RICH_READ_EVENT procedure                           *
 *                                                                      *
 ************************************************************************/

int rich_read_event_ (int* adc_counts, int* tdc, int* event_num, 
                      int* recl_adc, int* recl_tdc, int* date, int* time)
{
  int reply;

  fread ((void*)&record_type, 4, 1, fd);
  if (OSF1) {
    swap_bytes_l4 ((void*)&record_type);
  }
  if (DEBUG) {
    printf ("Record type = %x \n",record_type);
  }
  if      (record_type == 0xffff0005) {
    if (DEBUG) {
      printf (" $$$ Constant record $$$ \n");
    }
    read_constant_record;
    return 1; }
  else if (record_type == 0xffff0002) {
    if (DEBUG) {
      printf (" $$$ Event record $$$ \n");
    }
    reply = read_event_record (adc_counts, tdc, event_num, recl_adc, recl_tdc, date, time);

    if (reply) {
      return -1;
    }

    if (DEBUG) {
      printf (" Hello \n");
      printf (" Event Number %d \n", *event_num);
      printf (" Record Length ADC %d \n", *recl_adc);
      printf (" Record Length TDC %d \n", *recl_tdc);
      printf (" Date Event %d \n", *date);
      printf (" Time Event %d \n", *time);
    }
    return 0; }
  else if (record_type == 0xffff0003) {
    if (DEBUG) {
      printf (" $$$ End RUN record $$$ \n");
    }
    read_endrun_record;
    return 2; }
  else {
    printf ("Record of unknown type %x\n", record_type);
    return -1;
  }
}


void rich_close_run_ ()
{
   close(fd);
}


