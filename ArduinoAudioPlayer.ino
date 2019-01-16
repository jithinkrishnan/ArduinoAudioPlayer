/************************************************************************
*   Arduino Audio Player with Remote
*   
*   File:   ArduinoAudioPlayer.ino
*   Author:  Jithin Krishnan.K
*       Rev. 1.1 : 14/01/2019 :  10:49 PM
* 
* This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Email: jithinkrishnan.k@gmail.com
*   
************************************************************************/
#include <IRremote.h>
#include "SD.h"
#include "TMRpcm.h"
#include "SPI.h"

#define RECV_PIN 2

#define CONFIG_FILE "CONFIG.TXT"
#define SD_ChipSelectPin 10

TMRpcm tmrpcm;
IRrecv irrecv(RECV_PIN);
File root, file;
decode_results results;

int file_pos = 0,file_pos_n = 0;
String buf2;
char buf[20];
int fcnt = 0;
int playStop = 0;
int ir_buf[3];
int ir_buf_idx = 0;

void setup() {
 
  Serial.begin(115200);
  while (!Serial);
  pinMode(8, OUTPUT);
  tmrpcm.speakerPin = 9;
  attachInterrupt(digitalPinToInterrupt(2),CheckIRInterrupt,HIGH);
  irrecv.enableIRIn();
  tmrpcm.setVolume(5);
  tmrpcm.quality(1);
 
  if (!SD.begin(SD_ChipSelectPin)) {
    while(1);
   }
   
   root = SD.open("/");
  
    if (SD.exists(CONFIG_FILE)) {
        SD.remove(CONFIG_FILE);
    }
    
  CreateConfigFile(root);
  GetFileCount();
 }

void loop() {
  
  if (playStop) {
      Serial.println("playback stopped");
      tmrpcm.stopPlayback();
      playStop = 0; 
      delay(1000);
   } else if (!playStop) {
        if (!tmrpcm.isPlaying()) {
        buf2 =  GetFileName(file_pos);
        memset(buf, 0, sizeof(buf));
        buf2.toCharArray(buf, 20);
        Serial.print("Playing = ");
        Serial.println(buf);   
        tmrpcm.play(buf);
              
          if(file_pos < fcnt) // For playing in loop mode
              file_pos++;
            else
              file_pos = 0;  
          }     
        }
 }

// This fun will Create "CONFIG.TXT" file
// Content: the List of all wave files in 
// the root directory 
void CreateConfigFile(File dir)
{
  File ConfigFile;
  ConfigFile = SD.open(CONFIG_FILE, FILE_WRITE);
  
  while (true) {
    File entry =  dir.openNextFile();
    if (!entry) {
         break;
    }
    
    if (!entry.isDirectory()) {
      if (!strcmp(GetFileExt(entry.name()), "WAV"))  {
          memset(buf, 0, sizeof(buf));
          sprintf(buf, "%s\n", entry.name());
          ConfigFile.print(buf);
      }
    }
    entry.close();
  }
  ConfigFile.close();
}

// Return the wav file name with the arg index (idx) 
String GetFileName(int idx)
{
      File  ConfigFile;
      String str = "";
      int index_file = 0;
      
      ConfigFile = SD.open(CONFIG_FILE);

      while(ConfigFile.available()) {
      str = ConfigFile.readStringUntil('\n');
      
      if(str) {
       if (index_file == idx) {
           break;
        } else {
          if(index_file <= fcnt)
              index_file++;
          else 
              index_file = fcnt;    
        }
       }
      }
      ConfigFile.close();
      if(str)  
        return str;
      else
        return "";
}

const char *GetFileExt(const char *filename)
 {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) 
      return "";
    
     return dot + 1;
}

void GetFileCount(void)
{
      File  ConfigFile;
         
      ConfigFile = SD.open(CONFIG_FILE);
      
      while(ConfigFile.available()) {
          if(ConfigFile.readStringUntil('\n'))
          fcnt++;
        } 
 
      ConfigFile.close();
      return 0;
}

void CheckIRInterrupt() {

  if (irrecv.decode(&results)) {
  if (results.value == 0x1FEC03F) { // vire remote next button
       if(file_pos < fcnt)
          file_pos++;
       else
          file_pos = fcnt;  
          playStop = 1; 
          Serial.println("Playing Next File");      
      } else if (results.value == 0x1FE40BF) {// vire remote prev button 
       if(file_pos > 0)
          file_pos--;
       else
          file_pos = 0;
          playStop = 1;
          Serial.println("Playing Prev File");
      }  else if (results.value == 0x1FE807F) { // vire remote PLAY/PAUSE
          if(ir_buf_idx == 1)
            file_pos_n = ir_buf[0];
          else if (ir_buf_idx == 2)
            file_pos_n = ir_buf[0]*10 + ir_buf[1];
          else if (ir_buf_idx == 3)
            file_pos_n = ir_buf[0]*100 + ir_buf[1]*10 + ir_buf[2];
            ir_buf_idx = 0;
            
          if(file_pos_n < fcnt) {
            Serial.print("Playing File ");
            Serial.println(file_pos_n);
            file_pos = file_pos_n;
            playStop = 1;
          }
      }      
        switch(results.value) {
          case 0x1FEE01F: // 0
          ir_buf[ir_buf_idx] = 0;
          if (ir_buf_idx < 3)
              ir_buf_idx++;
              else
              ir_buf_idx = 0;
          break;
          case 0x1FE50AF:
          ir_buf[ir_buf_idx] = 1;
          if (ir_buf_idx < 3)
              ir_buf_idx++;
              else
              ir_buf_idx = 0;
          break;
          case 0x1FED827:
          ir_buf[ir_buf_idx] = 2;
          if (ir_buf_idx < 3)
              ir_buf_idx++;
              else
              ir_buf_idx = 0;
          break;
          case 0x1FEF807:
          ir_buf[ir_buf_idx] = 3;
          if (ir_buf_idx < 3)
              ir_buf_idx++;
              else
              ir_buf_idx = 0;
          break;
          case 0x1FE30CF:
          ir_buf[ir_buf_idx] = 4;
          if (ir_buf_idx < 3)
              ir_buf_idx++;
              else
              ir_buf_idx = 0;
          break;
          case 0x1FEB04F:
          ir_buf[ir_buf_idx] = 5;
          if (ir_buf_idx < 3)
              ir_buf_idx++;
              else
              ir_buf_idx = 0;
          break;
          case 0x1FE708F:
          ir_buf[ir_buf_idx] = 6;
          if (ir_buf_idx < 3)
              ir_buf_idx++;
              else
              ir_buf_idx = 0;
          break;
          case 0x1FE00FF:
          ir_buf[ir_buf_idx] = 7;
          if (ir_buf_idx < 3)
              ir_buf_idx++;
              else
              ir_buf_idx = 0;
          break;
          case 0x1FEF00F:
          ir_buf[ir_buf_idx] = 8;
          if (ir_buf_idx < 3)
              ir_buf_idx++;
              else
              ir_buf_idx = 0;
          break;
          case 0x1FE9867: // 9
          ir_buf[ir_buf_idx] = 9;
          if (ir_buf_idx < 3)
              ir_buf_idx++;
              else
              ir_buf_idx = 0;
          break;
          default:
          break;      
        }
           irrecv.resume();
        }
}
