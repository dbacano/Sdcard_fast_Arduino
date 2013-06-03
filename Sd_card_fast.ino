//#define DEBUG // uncomment to debug to serial

//libraries
#include <stdlib.h>
#include <SdFat.h>
#include <SdFatUtil.h>

//define pin configuration on arduino
const int lvd1=0;
const int lvd2=1;
const int lvd3=2;
const int lvd4=3;
const int sped=5;
const int curve=6;

const int Logled_on_off=7;
//initialize all variables and their values

int lvdt1 = 0;
int lvdt2 = 0;
int lvdt3 = 0;
int lvdt4 = 0;
int speeding = 0;

int xgg=0;

unsigned long t_absolute=0; //time since the power up of the arduino board
volatile int state = LOW; //logging switch on or off, using interrupts allows on less analog read during the loop, saving execution time, speeding other readings
volatile int finish = LOW; //variable for controlling the end of each log session
int apont = 0;
unsigned long numWrites = 0;
int inicio=0;

//SD card variables
uint32_t t = 0;
uint16_t maxWriteTime = 0;
uint32_t tw = 0;

uint32_t bgnBlock, endBlock;

// number of blocks in the contiguous file
#define BLOCK_COUNT 80000UL  // size of the file created (80000UL = 40Mb)
#define num_bytes_bloco 512 

Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file;

char str_t[100], str_max[100], str_num[100];

// store error strings in flash to save RAM
#define error(s) error_P(PSTR(s))

void error_P(const char *str)
{  
  #ifdef DEBUG // if debug
  PgmPrint("error: ");
  SerialPrintln_P(str);
  if (card.errorCode()) {
    PgmPrint("SD error: ");
    Serial.print(card.errorCode(), HEX);
    Serial.print(',');
    Serial.println(card.errorData(), HEX);
  }
  #endif
  digitalWrite(Logled_on_off,LOW);
}
uint8_t *pCache = volume.cacheClear();

//----------------------------------------------------------------------------------------
//                                         Setup
//----------------------------------------------------------------------------------------

void setup() {
  
  pinMode(9, OUTPUT);                      
  pinMode(10, OUTPUT);                       // set the SS pin as an output (necessary!)
  digitalWrite(10, HIGH);                    // but turn off the W5100 chip!
  
  uint8_t r = card.init(SPI_FULL_SPEED, 4);  // Use digital 4 as the SD SS line
  
  pinMode(Logled_on_off, OUTPUT);
  interrupts();
  attachInterrupt(1, botao_on_off, RISING);  //assign the button to corresponding interrupt pin (18) with interrupts triggered by RISING in pin value (LOW to HIGH)

  #ifdef DEBUG 
  Serial.begin(115200);
  Serial.println("Debug mode on!");
  Serial.println("");
  #endif
  
  #ifdef DEBUG
  Serial.println("Ready");
  #endif
  
}

//----------------------------------------------------------------------------------------
//                                         Loop
//----------------------------------------------------------------------------------------

void loop() {
  t_absolute=millis();
  
  if ((inicio==0) && (state==HIGH)){// initialize the SD card
    t = millis();
    if (!card.init()) {

      error("card.init");
    }

    // initialize a FAT volume
    if (!volume.init(card)) {
      error("volume.init");
    }

    // open the root directory
    if (!root.openRoot(volume)) { 
      error("openRoot");
    }

    //verificação do nome do ficheiro para nao haver ficheiros sobrepostos   
    char name[] = "RAW00.DAT";
    char nome[] = "RAW00.DAT";
    for (uint8_t i = 0; i < 100; i++) {
      name[3] = i/10 + '0';
      nome[3] = i/10 + '0';
      name[4] = i%10 + '0';
      nome[4] = i%10 + '0';
      // only create new file for write
      if (file.open(root, name, O_CREAT | O_EXCL | O_WRITE)) break;//tenta abrir um ficheiro com o nome e apenas tem sucesso quando o ficheiro nao existe
    }
    if (!file.isOpen()) {
      error ("duplicateverification.fileopen");
    }
    if (!file.close()) {
      error ("duplicateverification.fileclose");
    }
    // delete possible existing file
    SdFile::remove(root, name); //apos verificação do nome a usar para não haver sobreposiçoes, remove o ficheiro para o abrir em modo de blocos contiguos.
    // create a contiguous file
    if (!file.createContiguous(root, nome, 512UL*BLOCK_COUNT)) {
      error("createfile");
    }
   
    // get the location of the file's blocks
    if (!file.contiguousRange(bgnBlock, endBlock)) {
      error("contiguousRange");
    }
    
    // tell card to setup for multiple block write with pre-erase
    if (!card.erase(bgnBlock, endBlock)) {
      error("erase");  
    }
    if (!card.writeStart(bgnBlock, BLOCK_COUNT)) {
      error("writeStart");
    }
    
    #ifdef DEBUG
    Serial.println("Card setup complete");
    #endif
    
    inicio=1;
    
  } 
  
  if ((state==HIGH) && (inicio==1)){ 
    xgg=digitalRead(curve);
    //leitura de parametros
    lvdt1=analogRead(lvd1);
    lvdt2=analogRead(lvd2);
    lvdt3=analogRead(lvd3);
    lvdt4=analogRead(lvd4);
    speeding=analogRead(sped);

    #ifdef DEBUG
    Serial.print(lvdt1);
    Serial.print(" ");
    Serial.print(lvdt2);
    Serial.print(" ");
    Serial.print(lvdt3);
    Serial.print(" ");
    Serial.print(lvdt4);
    Serial.print(" ");
    Serial.print(speeding);
    Serial.print(" ");
    Serial.println(t_absolute);
    #endif
 
   if (apont==0){
       // clear the cache and use it as a num_bytes_bloco byte buffer
       memset(pCache, ' ', num_bytes_bloco);
    }
     
   // save data in cache 
   pCache[0+apont]=0xff; //synchro Byte
   
   pCache[1+apont]=((lvdt1 >> 5) & 0x1f) & 0xff;
   pCache[2+apont]=(lvdt1 & 0x1f) &0xff;
   pCache[3+apont]=((lvdt2 >> 5) & 0x1f) & 0xff;
   pCache[4+apont]=(lvdt2 & 0x1f) &0xff;
   pCache[5+apont]=((lvdt3 >> 5) & 0x1f) & 0xff;
   pCache[6+apont]=(lvdt3 & 0x1f) &0xff;
   pCache[7+apont]=((lvdt4 >> 5) & 0x1f) & 0xff;
   pCache[8+apont]=(lvdt4 & 0x1f) &0xff;
   pCache[9+apont]=((speeding >> 5) & 0x1f) & 0xff;
   pCache[10+apont]=(speeding & 0x1f) &0xff;
   
   pCache[11+apont]=(xgg >> 16) & 0xff;
   pCache[12+apont]=(xgg >> 8) & 0xff;
   pCache[13+apont]=(xgg) &0xff;
   
   pCache[14+apont]=(t_absolute >> 16) & 0xff;
   pCache[15+apont]=(t_absolute >> 8) & 0xff;
   pCache[16+apont]=(t_absolute) &0xff;

   apont+=17; //pointer for the next loop
   
   //verify if it is full
   if (apont+17>num_bytes_bloco){
   
      // write a num_bytes_bloco byte block to SD card
      tw = millis();
      if (!card.writeData(pCache)) {
        erro=3;
        error("writeData");    
      }
      tw = millis() - tw;
      // check for max write time
      if (tw > maxWriteTime) {
        maxWriteTime = tw;
      }  
      numWrites++;
      //fim da escrita
      apont=0;
      
      //verifying the remaining capacity
      if (numWrites > BLOCK_COUNT-2){
        #ifdef DEBUG
          Serial.println("Atencao: o ficheiro de dados atingiu o limite da capacidade!");
          Serial.print("Capacidade actual: ");
          Serial.print(BLOCK_COUNT);
          Serial.println(" blocos");
          Serial.println("A sessao sera terminada");
        #endif
        
        state=LOW;
        finish=HIGH;
      }

   } 
    
  } 
  else
  { 
    
    if (finish == HIGH){
        tw = millis();
        if (!card.writeData(pCache)) {  
          erro=3;
          error("writeData");  
        }

        // close files  
        root.close();
        file.close();
        
        #ifdef DEBUG
        Serial.println(" ");
        Serial.println("Log Stop");
        Serial.println(" ");
        Serial.println("done");
        #endif
        digitalWrite(Logled_on_off,state);
        finish=LOW;
        
    }
  } 
}

//----------------------------------------------------------------------------------------
//                                       Interrupt
//----------------------------------------------------------------------------------------

void botao_on_off() {
  //debouncing the interrupt button
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
//  // If interrupts comes faster than 500ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 500) {
    state = !state; 
    
    if (state==HIGH){
      
      #ifdef DEBUG
      Serial.println(" ");
      Serial.println("Log Start");
      #endif
      
      inicio=0;
      digitalWrite(Logled_on_off,state);
    }
    
    else{
      finish=HIGH;
    }
   }
last_interrupt_time = interrupt_time;
}

