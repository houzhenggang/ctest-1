/*-----------------------------------------------------------------

This program expects to control mplayer running in slave mode.

Environment Setup include:
-- lirc module  
-- espeak
-- mplayer 
-- rtl_sdr

Note: If you put lirc_mplay in rc.local for auto start-up,then you 
shall copy all required shell scripts into /bin/,otherwise they may 
not be activated. 
 
-----------------------------------------------------------------*/
#include <stdio.h>
//#include <sys/socket.h>  //connect,send,recv,setsockopt
//#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>  //sockaddr_in,htons..
//#include <netinet/tcp.h>
#include <unistd.h> //read,write
#include <stdlib.h> //--exit()
#include <errno.h>
#include <fcntl.h> //---open()
#include <string.h> //---strcpy()
#include <stdbool.h> // true,false

#define CODE_NUM_0 22
#define CODE_NUM_1 12
#define CODE_NUM_2 24
#define CODE_NUM_3 94
#define CODE_NUM_4 8
#define CODE_NUM_5 28
#define CODE_NUM_6 90
#define CODE_NUM_7 66
#define CODE_NUM_8 82
#define CODE_NUM_9 74

#define CODE_SHUTDOWN 69
#define CODE_MODE 70
#define CODE_MUTE 71
#define CODE_EQ   7
#define CODE_NEXT 67
#define CODE_PREV 64
#define CODE_PLAY_PAUSE 68
#define CODE_VOLUME_UP 9
#define CODE_VOLUME_DOWN 21
#define CODE_RELOAD 25 //---- 2 arrows 

#define MAXLINE 100
#define List_Item_Max_Num 15
#define MODE_MPLAYER 0
#define MODE_RADIO 1
#define MODE_AIRBAND 2
#define MODE_XIAMEN 3


char str_dev[]="/dev/LIRC_dev";
char str_radio_list[]="/mplayer/radio.list";
char str_xiamen_list[]="/mplayer/xiamen.list";
unsigned int PLAY_LIST_NUM=2; //---default playlist

void espeak_channel(int n) //this func will clog
{
    char strCMD[100];
    sprintf(strCMD,"speakE Playing-channel-%d",n);
    printf("%S\n",strCMD);
    system(strCMD);    
    //usleep(1500000); //--wait till espeak finish
}

void kill_espeak(void)
{
system("killall -9 espeak");
}

void kill_fm(void)
{
   system("killall -9 fm");
//   system("killall -9 rtl_fm");
//   system("killall -9 aplay");
}
void kill_mplay(void)
{
    system("killall -9 mplay");
//    system("killall -9 mplayer1");
//    system("killall -9 reaplay");
//    system("killall -9 aplay "); 
}
void kill_am(void)
{
   system("killall -9 am");
//   system("killall -9 rtl_fm");
//   system("killall -9 aplay");
}


void play_mplayer(void)
{
    char strCMD[100];
    kill_fm();
    kill_am();
    kill_mplay();
    kill_espeak();
    usleep(300000);
    system("speakE 'Sstart-to-play--Radio-Playlist'");    
    usleep(2500000); //--wait till espeak finish
    //sprintf(strCMD,"screen -dmS MPLAYER /mplayer/mplay -playlist  %s",str_radio_list);
    sprintf(strCMD,"screen -dmS MPLAYER /mplayer/mplay -playlist  %s",str_radio_list);
    printf("%s \n",strCMD);
    system(strCMD);
}

void play_xiamen(void)
{
    char strCMD[100];
    kill_fm();
    kill_am();
    kill_mplay();
    kill_espeak();
    usleep(300000);
    system("speakE Sstart-to-play--Radio-XM");    
    usleep(1600000);
    sprintf(strCMD,"screen -dmS MPLAYER /mplayer/mplay -aid 2 -playlist  %s",str_xiamen_list);
    printf("%s \n",strCMD);
    system(strCMD);
}

void  play_fm(float freq)
{
    char strCMD[50];
    char strSPEAK[50];
    kill_mplay();
    kill_am();
    kill_fm();
    kill_espeak();
    usleep(300000);
    sprintf(strSPEAK,"speakE 'fFM-%3.1f-Megahertz'",freq);
    system(strSPEAK);//--system run in shell as a thread    
    sprintf(strCMD,"screen -dmS FM /mplayer/fm %6.2f",freq);
    usleep(2500000);//--wait for espeak to release aplay
    system(strCMD);
    printf("%s\n",strCMD);
}

void  play_am(void)
{
    char strCMD[50];
    kill_mplay();
    kill_fm();
    kill_espeak();
    usleep(300000);
    system("speakE Sstart-to-play--Air-band");    
    usleep(2500000);
    sprintf(strCMD,"screen -dmS AM /mplayer/am");
    system(strCMD);
    printf("%s\n",strCMD);
}

void shut_down(void)
{
    kill_am();
    kill_fm();
    kill_mplay();
    system("speakE Sshut-Down");    
    usleep(1500000);

}

//====================================    main   =================================
int main(int argc, char** argv)
{

//----------- LIRC ------------
   int fd;
   int play_mode=0; // 0-mplayer 1-radio
   int nList_Item=1; //--List Item Number
   int nList_Gap=0; //--- difference between selected item number and current item number.
   bool flag_3D=false; //----3D effect sound
   unsigned int LIRC_DATA = 0; //---raw data from LIRC module 
   unsigned int LIRC_CODE =0;
   unsigned int volume_val=100;
   char strCMD[50];
   float FM_FREQ[10];
   FM_FREQ[0]=87.9;FM_FREQ[1]=89.9;FM_FREQ[2]=91.4;FM_FREQ[3]=93.4;FM_FREQ[4]=97.7;
   FM_FREQ[5]=99.1;FM_FREQ[6]=101.7;FM_FREQ[7]=103.7;FM_FREQ[8]=105.7;FM_FREQ[9]=107.7;
   unsigned int num_Freq=9;

//---------------  OPEN LIRC DEVICE   --------------
    fd = open(str_dev, O_RDWR | O_NONBLOCK);
    if (fd < 0)
     {
        printf("can't open %sn",str_dev);
        return -1;
      }

//------------------------- loop for input command and receive response ---------------------
while(1)
  {
    //--------------- receive LIRC data ---------
    read(fd, &LIRC_DATA, sizeof(LIRC_DATA));
    if(LIRC_DATA!=0)
    {
      printf("LIRC_DATA: 0x%0x\n",LIRC_DATA);  
      LIRC_CODE=(LIRC_DATA>>16)&0x000000ff;
      printf("LIRC_CODE: %d\n",LIRC_CODE);

      if(LIRC_CODE==CODE_MODE)
        {
          if(play_mode<3)
             play_mode+=1; //------ shift play_mode value         
          else
             play_mode=0;   

           //--------------------------------SELECT PLAY MODE --------------------------
           if(play_mode==MODE_MPLAYER)
              {   
                 printf("------- shift to MPLAYER SLAVE mode \n");
                 play_mplayer();
                 nList_Item=1;
               }
           else if(play_mode==MODE_XIAMEN)
              {   
                 printf("------- shift to XIAMEN-RADIO MPLAYER SLAVE mode \n");
                 play_xiamen();
                 nList_Item=1;
               }
            else if(play_mode==MODE_RADIO)
               {
                 printf("-------  shift to SDR FM RADIO mode \n");
                 play_fm(FM_FREQ[num_Freq]);
                }
            else if(play_mode==MODE_AIRBAND)
               {
                 printf("-------  shift to SDR AIR-BAND receiver mode \n");
                 play_am();
                }
           continue;
        }

      if(LIRC_CODE==CODE_SHUTDOWN) //----------  SHUT DOWN ---------
        {
           shut_down();
           printf("--------  SHUT DOWN NOW -------- \n");

            continue;
        }


      if(LIRC_CODE==CODE_VOLUME_UP) //----------  VOLUME ADJUST ---------
        {
            if(volume_val<125)
                  volume_val+=2;
             sprintf(strCMD,"amixer set Speaker %d",volume_val);
             system(strCMD);
             printf("%s \n",strCMD);
             continue;
         }
      if(LIRC_CODE==CODE_VOLUME_DOWN)
        {
              if(volume_val>20)
                   volume_val-=2;
              sprintf(strCMD,"amixer set Speaker %d",volume_val);
              system(strCMD);
              printf("%s \n",strCMD);
              continue;
        } 


       //-----!! WARNING use {} for every switch,case and default structre, or compilation will fail.    
      switch(play_mode)
      {
        case MODE_MPLAYER: //-----------------------------------for  MPLAYER ------------------------
        {
         printf("-------  MPLAYER SLAVE mode --------\n");
           switch(LIRC_CODE)
           {
               case CODE_NEXT:
                    if(nList_Item<List_Item_Max_Num)
                       nList_Item+=1; 
                    espeak_channel(nList_Item);  
 		    system("echo 'pt_step 1'>/mplayer/slave");
 		    printf("echo 'pt_step 1'>/mplayer/slave \n");
                    printf("nList_Item=%d\n",nList_Item);
     
                    break;
               case CODE_PREV:
                    if(nList_Item>1)
                       nList_Item-=1; 
                    espeak_channel(nList_Item);  
 		    system("echo 'pt_step -1'>/mplayer/slave");
 		    printf("echo 'pt_step -1'>/mplayer/slave \n");
                    printf("nList_Item=%d\n",nList_Item);
                    break;
               case CODE_PLAY_PAUSE:
                    system("echo 'pause'>/mplayer/slave");
 		    printf("echo 'pause'>/mplayer/slave \n");
                     break;
/*
               case CODE_VOLUME_UP: 
                    if(volume_val<125)
                        volume_val+=2;
                    sprintf(strCMD,"amixer set Speaker %d",volume_val);
                    system(strCMD);
                    printf("%s \n",strCMD);
                    break;
               case CODE_VOLUME_DOWN: 
                    if(volume_val>20)
                        volume_val-=2;
                    sprintf(strCMD,"amixer set Speaker %d",volume_val);
                    system(strCMD);
                    printf("%s \n",strCMD);
                    break; 
*/
      	       case CODE_EQ:
		    if(flag_3D)
                    {
         		system("amixer set 3D off");
                        printf("amixer set 3D off \n");
                        flag_3D=false;
                    }
                    else
                    {   
                        system("amixer set 3D 12");
                        system("amixer set 3D on");
                        printf("amixer set 3D 12 & on \n");
                        flag_3D=true;	
                     }
                     break;
               case CODE_RELOAD: 
                    sprintf(strCMD,"echo 'loadlist %s'>/mplayer/slave",str_radio_list);
                    printf("%s \n",strCMD);
                    system(strCMD);
                    nList_Item=1;
                    break;
               case CODE_MUTE: 
                    system("echo 'mute'>/mplayer/slave");
                    printf("echo 'mute'>/mplayer/slave \n");
                    break;
               default: 
               {
                 switch(LIRC_CODE)
                 {
                     case CODE_NUM_1:nList_Gap=1-nList_Item;nList_Item=1;break;
                     case CODE_NUM_2:nList_Gap=2-nList_Item;nList_Item=2;break;
                     case CODE_NUM_3:nList_Gap=3-nList_Item;nList_Item=3;break;
                     case CODE_NUM_4:nList_Gap=4-nList_Item;nList_Item=4;break;
                     case CODE_NUM_5:nList_Gap=5-nList_Item;nList_Item=5;break;
                     case CODE_NUM_6:nList_Gap=6-nList_Item;nList_Item=6;break;
                     case CODE_NUM_7:nList_Gap=7-nList_Item;nList_Item=7;break;
                     case CODE_NUM_8:nList_Gap=8-nList_Item;nList_Item=8;break;
                     case CODE_NUM_9:nList_Gap=9-nList_Item;nList_Item=9;break;
                     default:
                        printf("Unrecognizable code! \n");
                        break;   
                 }
                if(nList_Gap!=0) //---still in default
                 {
                     sprintf(strCMD,"echo 'pt_step %d'>/mplayer/slave",nList_Gap);    
                     printf("nList_Item=%d\n",nList_Item);
                     printf("%s \n",strCMD);
                     espeak_channel(nList_Item);  
                     system(strCMD);
                     nList_Gap=0;
                 }
                // break;
              };break;//--default
        };//---switch (LIRC_CODE) end
       }; break; //---- case MODE_MPLAY end

        case MODE_RADIO: //--------------------------- for SDR FM RADIO ----------------------------------------
        {
         printf("-------  SDR RADIO mode  --------\n");
         switch(LIRC_CODE)
          {
              case CODE_NUM_0:num_Freq=0;break;
              case CODE_NUM_1:num_Freq=1;break;
              case CODE_NUM_2:num_Freq=2;break;
              case CODE_NUM_3:num_Freq=3;break;
              case CODE_NUM_4:num_Freq=4;break;
              case CODE_NUM_5:num_Freq=5;break;
              case CODE_NUM_6:num_Freq=6;break;
              case CODE_NUM_7:num_Freq=7;break;
              case CODE_NUM_8:num_Freq=8;break;
              case CODE_NUM_9:num_Freq=9;break;
              default:
                  {
                   printf("Unrecognizable code! \n");
                   continue; //----------
                  }
          }
         play_fm(FM_FREQ[num_Freq]);

        };break;//--  case MODE_RADIO end


        case MODE_AIRBAND: //--------------------------- for SDR AIR-BAND RECEIVER ----------------------------------------
        {
         printf("-------  SDR AIR-BAND RECEIVER mode  --------\n");

        };break;

        case MODE_XIAMEN: //----------------------------------- for XIAMEN RADIO ---------------------------------------------
        {
         printf("-------  XIAMEN RADIO mode  --------\n");
           switch(LIRC_CODE)
           {
               case CODE_NEXT:
                    if(nList_Item<List_Item_Max_Num)
                       nList_Item+=1; 
                    espeak_channel(nList_Item);  
 		    system("echo 'pt_step 1'>/mplayer/slave");
 		    printf("echo 'pt_step 1'>/mplayer/slave \n");
                    printf("nList_Item=%d\n",nList_Item);
                    break;
               case CODE_PREV:
                    if(nList_Item>1)
                       nList_Item-=1; 
                    espeak_channel(nList_Item);  
 		    system("echo 'pt_step -1'>/mplayer/slave");
 		    printf("echo 'pt_step -1'>/mplayer/slave \n");
                    printf("nList_Item=%d\n",nList_Item);
                    break;
               case CODE_PLAY_PAUSE:
                    system("echo 'pause'>/mplayer/slave");
 		    printf("echo 'pause'>/mplayer/slave \n");
                     break;
      	       case CODE_EQ:
		    if(flag_3D)
                    {
         		system("amixer set 3D off");
                        printf("amixer set 3D off \n");
                        flag_3D=false;
                    }
                    else
                    {   
                        system("amixer set 3D 12");
                        system("amixer set 3D on");
                        printf("amixer set 3D 12 & on \n");
                        flag_3D=true;	
                     }
                     break;
               case CODE_RELOAD: 
                    sprintf(strCMD,"echo 'loadlist %s'>/mplayer/slave",str_radio_list);
                    printf("%s \n",strCMD);
                    system(strCMD);
                    nList_Item=1;
                    break;
               case CODE_MUTE: 
                    system("echo 'mute'>/mplayer/slave");
                    printf("echo 'mute'>/mplayer/slave \n");
                    break;
               default: 
               {
                 switch(LIRC_CODE)
                 {
                     case CODE_NUM_1:nList_Gap=1-nList_Item;nList_Item=1;break;
                     case CODE_NUM_2:nList_Gap=2-nList_Item;nList_Item=2;break;
                     case CODE_NUM_3:nList_Gap=3-nList_Item;nList_Item=3;break;
                     case CODE_NUM_4:nList_Gap=4-nList_Item;nList_Item=4;break;
                     case CODE_NUM_5:nList_Gap=5-nList_Item;nList_Item=5;break;
                     default:
                        printf("Unrecognizable code! \n");
                        break;   
                 }
                if(nList_Gap!=0) //---still in default
                 {
                     sprintf(strCMD,"echo 'pt_step %d'>/mplayer/slave",nList_Gap);    
                     printf("nList_Item=%d\n",nList_Item);
                     printf("%s \n",strCMD);
                     espeak_channel(nList_Item);
                     system(strCMD);
                     nList_Gap=0;
                 }
                // break;
              };break;//--default
        };//---switch (LIRC_CODE) end
       }; break; //---- case MODE_XIAMEN end


        default: //-----------------------------------  default -----------------------------------------------
          usleep(250000);continue;

      } //--switch play_mode end
 
     }  //---if end
     else //---  LIRC_DATA==0
     {
        usleep(250000);
        continue;  //---- no LIRC data received
      }

    usleep(250000); //---sleep

  } //--end while()

  close(fd);
  return 0;

}
