/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/19
 */
#include "common.h"
#include "FIFOreqchannel.h"
#include <sys/wait.h>

using namespace std;

// datapt function
void data_at_pt(FIFORequestChannel*chan, int temp_pat, double temp_time, int temp_ECG)
{
  datamsg data (temp_pat,temp_time,temp_ECG);
  chan->cwrite(&data, sizeof(data));
  char* msg = chan->cread();
  cout << "The ECG data for this patient: " << temp_pat << " at time: " << temp_time <<" and in ECG: " << temp_ECG << " is "<< *(double *) msg << endl;
}

void data_all(FIFORequestChannel*chan, int temp_pat)
{
  ofstream out_file;
  string file_type = ".csv";
  string file = "x";
  file += to_string(temp_pat);
  file += file_type;
  string file_loc = "received/";
  file_loc += file;
  out_file.open(file_loc);
  for(double i = 0; i <= 59.996; i += .004)
  {
    //Column 1 Time
    out_file<<i<<",";
    //Column 2 data 1
    datamsg data1 (temp_pat,i,1);
    chan->cwrite(&data1, sizeof(data1));
    char* out1 = chan->cread();
    out_file <<*(double *) out1<<",";
    //Column 3 data 2
    datamsg data2 (temp_pat,i,2);
    chan->cwrite(&data2, sizeof(data2));
    char* out2 = chan->cread();
    out_file <<*(double *) out2 << endl;
  }
}

void copy_file(FIFORequestChannel*chan, string in_file)
{
  string out_loc = "received/";
  string out_file = "y";
  out_file += in_file;
  out_loc += out_file;
  fstream out_stream;
  out_stream.open(out_loc, fstream::out | fstream::binary);
  filemsg D_File(0,0);
  int size =  in_file.length() + sizeof(D_File);
  char* buffer = new char[size + 1];
  *(filemsg *) buffer = D_File;
  strcpy(buffer+sizeof(filemsg), in_file.c_str());
  chan->cwrite(buffer, size+1);
  char* out_size = chan->cread();
  memcpy(buffer, &D_File, sizeof(filemsg));
  size = *(int*) out_size;
  int start_pt = 0;

  while(size != 0)
  {
    char* fin_buff = new char[sizeof(filemsg) + sizeof(in_file)];

    if(size > MAX_MESSAGE) // if greater than 256 bytes
    {
      filemsg file_msg = filemsg(start_pt, MAX_MESSAGE);
      memcpy(fin_buff, &file_msg, sizeof(file_msg));
      chan->cwrite(fin_buff, sizeof(file_msg));
      char* fin_out = chan->cread();
      start_pt += MAX_MESSAGE;
      size -= MAX_MESSAGE;
      out_stream.write(fin_out,MAX_MESSAGE);
    }
    else // smaller than 256 bytes
    {
      filemsg file_msg = filemsg(start_pt, size);
      memcpy(fin_buff, &file_msg, sizeof(file_msg));
      chan->cwrite(fin_buff, sizeof(file_msg));
      char* fin_out = chan->cread();
      out_stream.write(fin_out,size);
      size = 0;
    }
  }
  out_stream.close();
}

string get_time_diff(struct timeval * tp1, struct timeval * tp2) {
  /* Returns a string containing the difference, in seconds and micro seconds, between two timevals. */
  long sec = tp2->tv_sec - tp1->tv_sec;
  long musec = tp2->tv_usec - tp1->tv_usec;
  if (musec < 0) {
    musec += (int)1e6;
    sec--;
  }
  stringstream ss;
  ss<< " [sec = "<< sec <<", musec = "<<musec<< "]";
  return ss.str();
}

// start of main
int main(int argc, char *argv[])
{
  int n = 100;    // default number of requests per "patient"
  int p = 15;		// number of patients
  srand(time_t(NULL));
  int option;
  int pat = 0;
  double time = 0;
  int ECG = 0;
  string file;
  bool file_run = false;
  bool newchan = false;
  while((option = getopt(argc, argv, "p:t:e:f:c"))!=-1)
  {
    switch (option)
    {
      case 'p':
      pat = atoi(optarg);
      //cout << pat << endl;
      break;
  
      case 't':
      time = atoi(optarg);
      break;

      case 'e':
      ECG = atoi(optarg);
      break;
  
      case 'f':
      file = optarg;
      file_run = true;
      //cout << file << endl;
      break;
      
      case 'c':
      newchan = true;
      /*if(newchan)
      {
          cout << "this works" << endl;
      }*/
      
      break;

    }
  }
  // shell
  pid_t  pid;
  pid = fork();
  if (pid == 0)
  {
    char * const input[] = {"/bin/sh","-c","./dataserver", NULL};
    execvp("/bin/sh", input);
  }

  FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);
  struct timeval tp_start, tp_end;

  //data at a pt function call
  if(pat != 0 && (time >= 0 && time <= 59.996)&&(ECG>0 && ECG<3))
  {
    
    gettimeofday(&tp_start, 0); // start timer
    data_at_pt(&chan,pat,time,ECG);
    gettimeofday(&tp_end, 0);   // stop timer
    cout<<"Time taken: "<< get_time_diff(&tp_start, &tp_end) << endl;
  }

  //data at all pts function call
  else if(pat !=0)
  {
    gettimeofday(&tp_start, 0); // start timer
    data_all(&chan,pat);
    gettimeofday(&tp_end, 0);   // stop timer
    cout<<"Time taken: "<< get_time_diff(&tp_start, &tp_end) << endl;
  }

  // copies entire file
  if (file_run == true)
  {
    gettimeofday(&tp_start, 0); // start timer
    copy_file(&chan,file);
    gettimeofday(&tp_end, 0);   // stop timer
    cout<<"Time taken: "<< get_time_diff(&tp_start, &tp_end) << endl;
  }

  // new channel + new channel test proof
  if (newchan == true)
  {
    new_channel_msg new_chan_msg;
    chan.cwrite(&new_chan_msg, sizeof(new_chan_msg));
    char* new_chan_name = chan.cread();
    cout << "Channel Name is: " << new_chan_name << endl;
    FIFORequestChannel new_chan (new_chan_name, FIFORequestChannel::CLIENT_SIDE);

    int temp_num1 = 10; // 1 to 15
    double temp_num2 = 59.004; // 0 to 59.996
    int temp_num3 = 2; // 1 or 2

    datamsg data (temp_num1, temp_num2, temp_num2);
    new_chan.cwrite(&data, sizeof(data));
    char* msg = new_chan.cread();
    cout << "The ECG data for this patient: " << temp_num1 << " at time: " << temp_num2 <<" and in ECG: " << temp_num2 << " is "<< *(double *) msg << endl;

    //closing new channel
    MESSAGE_TYPE m2 = QUIT_MSG;
    new_chan.cwrite (&m2, sizeof (MESSAGE_TYPE));
  }

  // closing the channel    
  MESSAGE_TYPE m = QUIT_MSG;
  chan.cwrite (&m, sizeof (MESSAGE_TYPE));
  wait(NULL);
   
}
